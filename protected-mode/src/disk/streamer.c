#include "streamer.h"
#include "memory/heap/kheap.h"
#include "config.h"


struct disk_stream* diskstreamer_new(int disk_id)
{
    struct disk* disk = disk_get(disk_id);
    if (!disk)
    {
        return 0;
    }

    struct disk_stream* streamer = kzalloc(sizeof(struct disk_stream));
    streamer->pos = 0;
    streamer->disk = disk;
    return streamer;
}

int diskstreamer_seek(struct disk_stream* stream, int pos)
{
    stream->pos = pos;
    return 0;
}

// e.g. with stream->pos = 0x201 (513)
int diskstreamer_read(struct disk_stream* stream, void* out, int total)
{
    int sector = stream->pos / PEACHOS_SECTOR_SIZE; // We will read sector 1
    int offset = stream->pos % PEACHOS_SECTOR_SIZE; // We end at byte 1 of sector

    char buf[PEACHOS_SECTOR_SIZE];

    // Load the sector into buf
    int res = disk_read_block(stream->disk, sector, 1, buf);
    if (res < 0)
    {
        goto out;
    }

    int total_to_read = total > PEACHOS_SECTOR_SIZE ? PEACHOS_SECTOR_SIZE : total;
    //  total_to_read = 512
    //  total_to_read = 1 <- recursion

    for (int i = 0; i < total_to_read; i++)
    {
        *(char*)out++ = buf[offset+i];
        // Read one byte from our buffer into the out <- recursion
    }

    // Adjust the stream
    stream->pos += total_to_read;
    //      pos  = 1025

    // True, do recursion
    if (total > PEACHOS_SECTOR_SIZE)
    {
        res = diskstreamer_read(stream, out, total-PEACHOS_SECTOR_SIZE);
        //    diskstreamer_read(stream, out, 1);
    }
out:
    return res;
}

void diskstreamer_close(struct disk_stream* stream)
{
    kfree(stream);
}