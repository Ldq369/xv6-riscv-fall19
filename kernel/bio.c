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

struct {
  //struct spinlock lock;
  struct buf buf[NBUF];
  struct spinlock lock[NBUCKETS];
  struct buf hashbucket[NBUCKETS];
  // Linked list of all buffers, through prev/next.
  // head.next is most recently used.
  //struct buf head;
} bcache;
uint
hash(uint blockno)
{
  return blockno % 13;
}
//构建循环双向链表
void
binit(void)
{
  struct buf *b;
  uint bucketno;
  //initlock(&bcache.lock, "bcache");//自旋锁锁住
  for(int i=0;i<NBUCKETS;i++)
  {
    initlock(&bcache.lock[i],"bcache");
  }
  
  // Create linked list of buffers
  for(int i=0;i<NBUCKETS;i++)
  {
    bcache.hashbucket[i].prev = &bcache.hashbucket[i];
    bcache.hashbucket[i].next = &bcache.hashbucket[i];
  }


  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    bucketno = hash(b->blockno);
    b->next = bcache.hashbucket[bucketno].next;
    b->prev = &bcache.hashbucket[bucketno];

    initsleeplock(&b->lock, "buffer");
    
    bcache.hashbucket[bucketno].next->prev = b;
    bcache.hashbucket[bucketno].next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  uint bucketno = hash(blockno);
  acquire(&bcache.lock[bucketno]);

  // 在blockno对应的哈希桶中查找是否命中

  for(b = bcache.hashbucket[bucketno].next; b != &bcache.hashbucket[bucketno]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.lock[bucketno]);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // 在blockno对应的哈希桶中没命中，在桶里找空闲的块
  for(b = bcache.hashbucket[bucketno].prev; b != &bcache.hashbucket[bucketno]; b = b->prev){
    if(b->refcnt == 0) {
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      release(&bcache.lock[bucketno]);
      acquiresleep(&b->lock);
      return b;
    } 
  }
  //当bget()查找数据块未命中时，bget可从其他哈希桶选择一个未被使用的缓存块，移入到当前的哈希桶链表中使用
  for(int j = 0; j< NBUCKETS; j++){
    while(j != bucketno){
      for(b = bcache.hashbucket[j].prev; b != &bcache.hashbucket[j]; b = b->prev){
        acquire(&bcache.lock[j]);
        if(b->refcnt == 0){
          b->dev = dev;
          b->blockno = blockno;
          b->valid = 0;
          b->refcnt = 1;
          //断开
          b->prev->next=b->next;
          b->next->prev=b->prev;
          //连起来
          b->next = bcache.hashbucket[bucketno].next;
          b->prev = &bcache.hashbucket[bucketno];
          bcache.hashbucket[bucketno].next->prev = b;
          bcache.hashbucket[bucketno].next = b;
          release(&bcache.lock[j]);
          release(&bcache.lock[bucketno]);
          acquiresleep(&b->lock);
          return b;
        } 
        release(&bcache.lock[j]);
      }
    }
  }
  release(&bcache.lock[bucketno]);
  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b->dev, b, 0);
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
  virtio_disk_rw(b->dev, b, 1);
}

// Release a locked buffer.
// Move to the head of the MRU list.
void
brelse(struct buf *b)
{
  uint bucketno = hash(b->blockno);
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  acquire(&bcache.lock[bucketno]);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.hashbucket[bucketno].next;
    b->prev = &bcache.hashbucket[bucketno];
    bcache.hashbucket[bucketno].next->prev = b;
    bcache.hashbucket[bucketno].next = b;
  }
  
  release(&bcache.lock[bucketno]);
}

void
bpin(struct buf *b) {
  uint bucketno = hash(b->blockno);
  acquire(&bcache.lock[bucketno]);
  b->refcnt++;
  release(&bcache.lock[bucketno]);
}

void
bunpin(struct buf *b) {
  uint bucketno = hash(b->blockno);
  acquire(&bcache.lock[bucketno]);
  b->refcnt--;
  release(&bcache.lock[bucketno]);
}


