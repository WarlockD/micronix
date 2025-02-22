/*
 * file i/o
 *
 * sys/fio.c 
 * Changed: <2021-12-24 05:57:11 curt>
 */
#include <types.h>
#include <sys/sys.h>
#include <sys/fs.h>
#include <sys/stat.h>
#include <sys/inode.h>
#include <sys/buf.h>
#include <sys/con.h>
#include <sys/proc.h>

extern long seconds;

/*
 * Read a file from its inode. The transfer parameters
 * are u.count, u.base, and u.offset.
 * Return the number of bytes actually read.
 * Called by the read system call (including pipes),
 * to read directories, and to load files for execution.
 */
iread(ip)
    register struct inode *ip;
{
    int request, dev, log, phys, rahead;
    register int into, nbytes;
    static int type;
    struct buf *bp;
    static long gap;

    if ((request = u.count) == 0)
        return (0);
    type = ip->i_mode & IFMT;
    dev = (type & IIO) ? ip->i_addr[0] : ip->i_dev;
    if (type == IFCHR) {
        cread(dev);
        goto done;
    }
    if (type != IFBLK) {         /* don't read past eof */
        if ((gap = ip->i_size - u.offset) <= 0)
            goto out;
        if (request > gap)
            request = u.count = gap;
    }
    log = u.offset >> 9;        /* divide by 512 */
    into = u.offset & 511;      /* mod 512 */
    while (u.count != 0) {
        phys = imap(ip, log, &rahead);
        if (u.error)
            break;
        nbytes = min(u.count, 512 - into);
        if (rahead != 0 && into + nbytes == 512)
            aread(rahead, dev);
        if ((bp = bread(phys, dev)) == 0)
            break;
        iomove(READ, bp->data + into, nbytes);
        brelse(bp);
        log++;
        into = 0;
    }
  done:
    if (!(ip->flags & IRONLY)) {
        ip->i_rtime = seconds;
        ip->i_flags |= IMOD;
    }
  out:
    return (request - u.count);
}

/*
 * Write a file from its inode. See iread comments above.
 * Used for system write calls, pipes, and directories.
 */
iwrite(ip)
    register struct inode *ip;
{
    int request, dev, log, phys, pipe;
    register int into, nbytes;
    static int type, dummy;
    struct buf *bp;

    if ((request = u.count) == 0)
        return (0);
    type = ip->i_mode & IFMT;
    pipe = ip->i_flags & IPIPE;
    dev = (type & IIO) ? ip->i_addr[0] : ip->i_dev;
    if (type == IFCHR) {
        cwrite(dev);
        goto done;
    }
    log = u.offset >> 9;        /* divide by 512 */
    into = u.offset & 511;      /* mod 512 */
    while (u.count != 0) {
        phys = imap(ip, log, &dummy);   /* rahead not used */
        if (u.error)
            break;
        if ((nbytes = min(u.count, 512 - into)) == 512)
            bp = bget(phys, dev);
        else if ((bp = bread(phys, dev)) == 0)
            break;
        iomove(WRITE, bp->data + into, nbytes);
        if ((into + nbytes == 512) && !pipe)
            bawrite(bp);
        else
            bdwrite(bp);
        log++;
        into = 0;
    }
  done:
    if (ip->size < u.offset)
        ip->size = u.offset;
    if (!(ip->flags & IRONLY)) {        /* special files */
        ip->i_mtime = seconds;
        ip->flags |= IMOD;
    }
  out:
    return (request - u.count);
}

/*
 * Iomove(flag, buf, count)
 * Copy count bytes from buf to u.base (flag == READ), or
 * from u.base to buf (flag == WRITE). U.base, u.count,
 * and u.offset are updated by count.
 *   Buf is always in kernel space.  U->base is usually in
 * user space (u.segflg == USER), but it may be in kernel
 * space (u.segflg == KSEG).
 */
iomove(flag, buf, count)
    int flag;
    register char *buf;
    register int count;
{
    u.count -= count;
    u.offset += count;
    if (u.segflg == KSEG) {
        if (flag == READ)
            copy(buf, u.base, count);
        else
            copy(u.base, buf, count);
    } else {
        if (flag == READ)
            copyout(buf, u.base, count);
        else
            copyin(u.base, buf, count);
    }
    u.base += count;
}

/*
 * Set u.base and u.count, then write.
 * Called by ilink() and unlink().
 * Assumes that u.offset and u.segflg
 * have been set elsewhere.
 */
nwrite(ip, base, count)
    struct inode *ip;
    char *base;
    UINT count;
{
    u.base = base;
    u.count = count;
    return (iwrite(ip));
}

/*
 * Set u.base and u.count, then read.
 * Called by exec() and name().
 * Assumes that u.offset and u.segflg
 * have been set elsewhere.
 */
nread(ip, base, count)
    struct inode *ip;
    char *base;
    UINT count;
{
    u.base = base;
    u.count = count;
    return (iread(ip));
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
