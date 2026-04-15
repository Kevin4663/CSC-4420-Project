/* This file contains functions that are not part of the visible interface.
 * So they are essentially helper functions.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "simfs.h"

#include <limits.h>

/* Internal helper functions first.
 */

FILE *
openfs(char *filename, char *mode)
{
    FILE *fp;
    if ((fp = fopen(filename, mode)) == NULL)
    {
        perror("openfs");
        exit(1);
    }
    return fp;
}

void closefs(FILE *fp)
{
    if (fclose(fp) != 0)
    {
        perror("closefs");
        exit(1);
    }
}

/* File system operations: creating, deleting, reading from, and writing to files.
 */

// Signatures omitted; design as you wish.

static int
read_metadata(FILE *fp, fentry files[], fnode fnodes[])
{
    if (fseek(fp, 0, SEEK_SET) != 0)
        return -1;
    if (fread(files, sizeof(fentry), MAXFILES, fp) != MAXFILES)
        return -1;
    if (fread(fnodes, sizeof(fnode), MAXBLOCKS, fp) != MAXBLOCKS)
        return -1;
    return 0;
}

static int
write_metadata(FILE *fp, fentry files[], fnode fnodes[])
{
    if (fseek(fp, 0, SEEK_SET) != 0)
        return -1;
    if (fwrite(files, sizeof(fentry), MAXFILES, fp) != MAXFILES)
        return -1;
    if (fwrite(fnodes, sizeof(fnode), MAXBLOCKS, fp) != MAXBLOCKS)
        return -1;
    return 0;
}

static long
data_region_offset(void)
{
    int bytes_used = sizeof(fentry) * MAXFILES + sizeof(fnode) * MAXBLOCKS;
    int bytes_to_write = (BLOCKSIZE - (bytes_used % BLOCKSIZE)) % BLOCKSIZE;
    return (long)(bytes_used + bytes_to_write);
}

static int
find_fentry_index(fentry files[], const char *name)
{
    int i;
    for (i = 0; i < MAXFILES; i++)
    {
        if (files[i].name[0] != '\0' && strcmp(files[i].name, name) == 0)
            return i;
    }
    return -1;
}

static int
find_free_fentry(fentry files[])
{
    int i;
    for (i = 0; i < MAXFILES; i++)
    {
        if (files[i].name[0] == '\0')
            return i;
    }
    return -1;
}

static int
count_free_blocks(fnode fnodes[])
{
    int i, cnt = 0;
    for (i = 0; i < MAXBLOCKS; i++)
        if (fnodes[i].blockindex < 0)
            cnt++;
    return cnt;
}

static int
allocate_blocks(fnode fnodes[], int needed, int alloc_list[])
{
    int i, k = 0;
    for (i = 0; i < MAXBLOCKS && k < needed; i++)
    {
        if (fnodes[i].blockindex < 0)
        {
            alloc_list[k++] = i;
        }
    }
    if (k < needed)
        return -1;
    /* mark allocated (but do not set next pointers here) */
    for (i = 0; i < needed; i++)
    {
        int idx = alloc_list[i];
        fnodes[idx].blockindex = idx;
        fnodes[idx].nextblock = -1;
    }
    return 0;
}

int create_file(char *fsname, char *fname)
{
    if (strlen(fname) > 11)
        return 1;
    fentry files[MAXFILES];
    fnode fnodes[MAXBLOCKS];
    FILE *fp = openfs(fsname, "r+");
    if (read_metadata(fp, files, fnodes) != 0)
    {
        closefs(fp);
        return 1;
    }

    if (find_fentry_index(files, fname) != -1)
    {
        closefs(fp);
        return 1;
    }
    int freeidx = find_free_fentry(files);
    if (freeidx < 0)
    {
        closefs(fp);
        return 1;
    }

    strncpy(files[freeidx].name, fname, sizeof(files[freeidx].name) - 1);
    files[freeidx].name[sizeof(files[freeidx].name) - 1] = '\0';
    files[freeidx].size = 0;
    files[freeidx].firstblock = -1;

    if (write_metadata(fp, files, fnodes) != 0)
    {
        closefs(fp);
        return 1;
    }
    closefs(fp);
    return 0;
}

int delete_file(char *fsname, char *fname)
{
    fentry files[MAXFILES];
    fnode fnodes[MAXBLOCKS];
    FILE *fp = openfs(fsname, "r+");
    if (read_metadata(fp, files, fnodes) != 0)
    {
        closefs(fp);
        return 1;
    }

    int idx = find_fentry_index(files, fname);
    if (idx < 0)
    {
        closefs(fp);
        return 1;
    }

    int cur = files[idx].firstblock;
    while (cur != -1)
    {
        int next = fnodes[cur].nextblock;
        fnodes[cur].blockindex = -cur;
        fnodes[cur].nextblock = -1;
        cur = next;
    }

    files[idx].name[0] = '\0';
    files[idx].size = 0;
    files[idx].firstblock = -1;

    if (write_metadata(fp, files, fnodes) != 0)
    {
        closefs(fp);
        return 1;
    }
    closefs(fp);
    return 0;
}

int write_file(char *fsname, char *fname, int offset, int count)
{
    if (count < 0 || offset < 0)
        return 1;
    fentry files[MAXFILES];
    fnode fnodes[MAXBLOCKS];
    FILE *fp = openfs(fsname, "r+");
    if (read_metadata(fp, files, fnodes) != 0)
    {
        closefs(fp);
        return 1;
    }

    int idx = find_fentry_index(files, fname);
    if (idx < 0)
    {
        closefs(fp);
        return 1;
    }

    unsigned short old_size = files[idx].size;
    if (offset > old_size)
    {
        closefs(fp);
        return 1;
    }

    /* Determine blocks needed */
    int old_blocks = (old_size + BLOCKSIZE - 1) / BLOCKSIZE;
    if (old_size == 0)
        old_blocks = 0;
    int new_size = old_size;
    if (offset + count > new_size)
        new_size = offset + count;
    int new_blocks = (new_size + BLOCKSIZE - 1) / BLOCKSIZE;
    if (new_size == 0)
        new_blocks = 0;
    int additional = new_blocks - old_blocks;

    if (additional > 0)
    {
        int freecnt = count_free_blocks(fnodes);
        if (freecnt < additional)
        {
            closefs(fp);
            return 1;
        }
    }

    /* Allocate additional blocks up front (atomic) */
    int alloc_list[MAXBLOCKS];
    if (additional > 0)
    {
        if (allocate_blocks(fnodes, additional, alloc_list) != 0)
        {
            closefs(fp);
            return 1;
        }
        /* Link allocated blocks into file chain */
        if (files[idx].firstblock == -1)
        {
            files[idx].firstblock = alloc_list[0];
            int k;
            for (k = 0; k < additional - 1; k++)
                fnodes[alloc_list[k]].nextblock = alloc_list[k + 1];
        }
        else
        {
            /* find tail */
            int cur = files[idx].firstblock;
            while (fnodes[cur].nextblock != -1)
                cur = fnodes[cur].nextblock;
            fnodes[cur].nextblock = alloc_list[0];
            int k;
            for (k = 0; k < additional - 1; k++)
                fnodes[alloc_list[k]].nextblock = alloc_list[k + 1];
        }
    }

    /* Read exactly `count` bytes from stdin into a buffer */
    char *buf = NULL;
    if (count > 0)
    {
        buf = malloc(count);
        if (!buf)
        {
            closefs(fp);
            return 1;
        }
        int read_total = 0;
        while (read_total < count)
        {
            int r = fread(buf + read_total, 1, count - read_total, stdin);
            if (r <= 0)
                break;
            read_total += r;
        }
        if (read_total != count)
        {
            free(buf);
            closefs(fp);
            return 1;
        }
    }

    /* Perform writes across blocks */
    long base = data_region_offset();
    int block_idx_in_file = offset / BLOCKSIZE;
    int inner = offset % BLOCKSIZE;
    int cur = files[idx].firstblock;
    int traversed = 0;
    while (traversed < block_idx_in_file && cur != -1)
    {
        cur = fnodes[cur].nextblock;
        traversed++;
    }
    if (traversed != block_idx_in_file)
    {
        free(buf);
        closefs(fp);
        return 1;
    }

    int remaining = count;
    int bufpos = 0;
    while (remaining > 0)
    {
        if (cur == -1)
        {
            free(buf);
            closefs(fp);
            return 1;
        }
        long block_offset = base + ((long)cur) * BLOCKSIZE + inner;
        if (fseek(fp, block_offset, SEEK_SET) != 0)
        {
            free(buf);
            closefs(fp);
            return 1;
        }
        int write_amount = BLOCKSIZE - inner;
        if (write_amount > remaining)
            write_amount = remaining;
        if (fwrite(buf + bufpos, 1, write_amount, fp) != (size_t)write_amount)
        {
            free(buf);
            closefs(fp);
            return 1;
        }
        remaining -= write_amount;
        bufpos += write_amount;
        inner = 0; /* only first block may have inner offset */
        cur = fnodes[cur].nextblock;
    }

    if (buf)
        free(buf);

    files[idx].size = (unsigned short)new_size;
    if (write_metadata(fp, files, fnodes) != 0)
    {
        closefs(fp);
        return 1;
    }
    closefs(fp);
    return 0;
}

int read_file(char *fsname, char *fname, int offset, int count)
{
    if (count < 0 || offset < 0)
        return 1;
    fentry files[MAXFILES];
    fnode fnodes[MAXBLOCKS];
    FILE *fp = openfs(fsname, "r");
    if (read_metadata(fp, files, fnodes) != 0)
    {
        closefs(fp);
        return 1;
    }

    int idx = find_fentry_index(files, fname);
    if (idx < 0)
    {
        closefs(fp);
        return 1;
    }
    unsigned short fsize = files[idx].size;
    if (offset >= fsize)
    {
        closefs(fp);
        return 1;
    }
    int can_read = fsize - offset;
    if (count > can_read)
        count = can_read;

    long base = data_region_offset();
    int block_idx_in_file = offset / BLOCKSIZE;
    int inner = offset % BLOCKSIZE;
    int cur = files[idx].firstblock;
    int traversed = 0;
    while (traversed < block_idx_in_file && cur != -1)
    {
        cur = fnodes[cur].nextblock;
        traversed++;
    }
    if (traversed != block_idx_in_file)
    {
        closefs(fp);
        return 1;
    }

    int remaining = count;
    char blockbuf[BLOCKSIZE];
    while (remaining > 0)
    {
        if (cur == -1)
        {
            closefs(fp);
            return 1;
        }
        long block_offset = base + ((long)cur) * BLOCKSIZE + inner;
        if (fseek(fp, block_offset, SEEK_SET) != 0)
        {
            closefs(fp);
            return 1;
        }
        int toread = BLOCKSIZE - inner;
        if (toread > remaining)
            toread = remaining;
        if (fread(blockbuf, 1, toread, fp) != (size_t)toread)
        {
            closefs(fp);
            return 1;
        }
        if (fwrite(blockbuf, 1, toread, stdout) != (size_t)toread)
        {
            closefs(fp);
            return 1;
        }
        remaining -= toread;
        inner = 0;
        cur = fnodes[cur].nextblock;
    }

    closefs(fp);
    return 0;
}
