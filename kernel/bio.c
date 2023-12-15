// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#define NBUCKET 13
struct {
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf buckethead[NBUCKET];
  struct spinlock bucketlock[NBUCKET];
} bcache;

uint hash(uint i){
  return i%NBUCKET;
}

void
binit(void)
{
  struct buf *b;

  for(int i = 0;i < NBUCKET;i++){
    initlock(&bcache.bucketlock[i], "bcache");
    bcache.buckethead[i].prev = &bcache.buckethead[i];
    bcache.buckethead[i].next = &bcache.buckethead[i];
  }

  // Create linked list of buffer[0]
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    b->next = bcache.buckethead[0].next;
    b->prev = &bcache.buckethead[0];
    initsleeplock(&b->lock, "buffer");
    bcache.buckethead[0].next->prev = b;
    bcache.buckethead[0].next = b;
  }

  for(int i=0;i<NBUF;i++){
    bcache.buf[i].refcnt=0;
  }

}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{

  struct buf *b;
  uint hashid;

  hashid = hash(blockno);

  // Is the block already cached?
  acquire(&bcache.bucketlock[hashid]);
  for(b = bcache.buckethead[hashid].next; b != &bcache.buckethead[hashid]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.bucketlock[hashid]);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  for(b = bcache.buckethead[hashid].next; b != &bcache.buckethead[hashid]; b = b->next){
    if(b->refcnt == 0) {
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      release(&bcache.bucketlock[hashid]);
      acquiresleep(&b->lock);
      return b;
    }
  }
  release(&bcache.bucketlock[hashid]);
  //allocate a new buffer for the bucket hashid
  for(int i=0;i < NBUCKET;i++){
    if(i == hashid)
      continue;
    acquire(&bcache.bucketlock[i]);
      for(b = bcache.buckethead[i].next; b != &bcache.buckethead[i]; b = b->next){
        if(b->refcnt == 0) {
          b->dev = dev;
          b->blockno = blockno;
          b->valid = 0;
          b->refcnt = 1;
          //insert into hashid bucket
          b->prev->next = b->next;
          b->next->prev = b->prev;
          b->next=0;
          b->prev=0;
          release(&bcache.bucketlock[i]);
          acquire(&bcache.bucketlock[hashid]);
          b->next = bcache.buckethead[hashid].next;
          b->prev = &bcache.buckethead[hashid];
          bcache.buckethead[hashid].next->prev = b;
          bcache.buckethead[hashid].next = b;
          release(&bcache.bucketlock[hashid]);
          acquiresleep(&b->lock);
          return b;
        }
    }
    release(&bcache.bucketlock[i]);
  }

  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  uint hashid;
  if(!holdingsleep(&b->lock))
    panic("brelse");
  
  hashid=hash(b->blockno);
  releasesleep(&b->lock);
  acquire(&bcache.bucketlock[hashid]);
  b->refcnt--;
  release(&bcache.bucketlock[hashid]);
}

void
bpin(struct buf *b) {
  uint hashid;

  hashid=hash(b->blockno);
  acquire(&bcache.bucketlock[hashid]);
  b->refcnt++;
  release(&bcache.bucketlock[hashid]);
}

void
bunpin(struct buf *b) {
  uint hashid;

  hashid=hash(b->blockno);
  acquire(&bcache.bucketlock[hashid]);
  b->refcnt--;
  release(&bcache.bucketlock[hashid]);
}


