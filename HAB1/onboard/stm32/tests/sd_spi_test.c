#include "../modules/drivers/sd_card/sd_spi.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    uint8_t reply[600];
    size_t head, tail;
    uint8_t block[512];
    bool selected, v2, fail, silent, fast, writing, reject_write, busy;
    unsigned write_position;
    uint32_t now, last_address;
} fake_t;
static void push(fake_t *fixture, uint8_t b)
{
    assert(fixture->tail < sizeof(fixture->reply));
    fixture->reply[fixture->tail++] = b;
}
static bool transfer(void *p, const uint8_t *tx, uint8_t *rx, size_t n)
{
    fake_t *fixture = p;
    ++fixture->now;
    if (fixture->fail) {
        return false;
    }
    memset(rx, 0xff, n);
    if (!fixture->selected || fixture->silent) {
        return true;
    }
    if (fixture->busy) {
        memset(rx, 0, n);
        return true;
    }
    if (n == 6 && (tx[0] & 0xc0) == 0x40 && !fixture->writing) {
        uint8_t cmd = tx[0] & 0x3f;
        uint32_t arg =
            ((uint32_t)tx[1] << 24) | ((uint32_t)tx[2] << 16) | ((uint32_t)tx[3] << 8) | tx[4];
        fixture->head = fixture->tail = 0;
        if (cmd == 0) {
            push(fixture, 1);
            assert(tx[5] == 0x95);
        } else if (cmd == 8) {
            push(fixture, fixture->v2 ? 1 : 5);
            if (fixture->v2) {
                push(fixture, 0);
                push(fixture, 0);
                push(fixture, 1);
                push(fixture, 0xaa);
            }
        } else if (cmd == 55 || cmd == 41) {
            push(fixture, 0);
        } else if (cmd == 58) {
            push(fixture, 0);
            push(fixture, fixture->v2 ? 0xc0 : 0x80);
            push(fixture, 0xff);
            push(fixture, 0x80);
            push(fixture, 0);
        } else if (cmd == 16) {
            assert(arg == 512);
            push(fixture, 0);
        } else if (cmd == 17) {
            fixture->last_address = arg;
            push(fixture, 0);
            push(fixture, 0xfe);
            for (unsigned i = 0; i < 512; ++i) {
                push(fixture, fixture->block[i]);
            }
            push(fixture, 0xff);
            push(fixture, 0xff);
        } else if (cmd == 24) {
            fixture->last_address = arg;
            push(fixture, 0);
            fixture->writing = true;
            fixture->write_position = 0;
        } else if (cmd == 13) {
            push(fixture, 0);
            push(fixture, 0);
        } else {
            assert(0);
        }
        return true;
    }
    for (size_t i = 0; i < n; ++i) {
        if (fixture->head < fixture->tail) {
            rx[i] = fixture->reply[fixture->head++];
            continue;
        }
        if (fixture->writing) {
            if (fixture->write_position == 0) {
                if (tx[i] == 0xfe) {
                    fixture->write_position = 1;
                }
            } else if (fixture->write_position <= 512) {
                fixture->block[fixture->write_position - 1] = tx[i];
                ++fixture->write_position;
            } else if (++fixture->write_position == 515) {
                fixture->head = fixture->tail = 0;
                push(fixture, fixture->reject_write ? 0x0d : 5);
                fixture->writing = false;
            }
        }
    }
    return true;
}
static void select_card(void *p, bool selected)
{
    fake_t *fixture = p;
    fixture->selected = selected;
    if (!selected) {
        fixture->head = fixture->tail = 0;
    }
}
static bool speed(void *p, bool fast)
{
    ((fake_t *)p)->fast = fast;
    return true;
}
static uint32_t clock_ms(void *p)
{
    return ((fake_t *)p)->now;
}
static void delay_ms(void *p, uint32_t ms)
{
    ((fake_t *)p)->now += ms;
}
int main(void)
{
    for (unsigned v2 = 0; v2 < 2; ++v2) {
        fake_t fixture = {0};
        fixture.v2 = v2 != 0;
        sd_spi_bus_t bus = {&fixture, transfer, select_card, speed, clock_ms, delay_ms};
        sd_spi_t c;
        assert(sd_spi_init(&c, &bus));
        assert(c.block_addressed == fixture.v2 && fixture.fast);
        uint8_t tx[512], rx[512];
        for (unsigned i = 0; i < 512; ++i) {
            tx[i] = (uint8_t)(i * 13);
        }
        assert(sd_spi_write(&c, 3, tx));
        assert(fixture.last_address == (v2 ? 3 : 1536));
        assert(sd_spi_read(&c, 3, rx));
        assert(memcmp(tx, rx, 512) == 0);
        assert(sd_spi_sync(&c));
        if (!v2) {
            assert(!sd_spi_read(&c, UINT32_MAX, rx));
        }
        fixture.reject_write = true;
        assert(!sd_spi_write(&c, 3, tx) && !c.initialized);
        fixture.reject_write = false;
        assert(sd_spi_init(&c, &bus));
        fixture.busy = true;
        assert(!sd_spi_sync(&c) && !c.initialized);
        fixture.busy = false;
        fixture.fail = true;
        assert(!sd_spi_init(&c, &bus));
        fixture.fail = false;
        fixture.silent = true;
        assert(!sd_spi_init(&c, &bus));
    }
    puts("PASS: SDSC/SDHC initialization, addressing, read/write, rejection and timeout");
    return 0;
}
