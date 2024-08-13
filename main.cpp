/*

Bradshaw Instruments
Pico Audio template code

*/

// Include a bunch of libraries (possibly not all necessary)
#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <stdio.h>
#include <math.h>
#include <array>
#include <algorithm>

// Include Pico-specific stuff and set up audio
#include "hardware/clocks.h"
#include "hardware/structs/clocks.h"
#include "hardware/adc.h"
#include "hardware/flash.h"
#include "hardware/irq.h"
#include "pico/stdlib.h"
#include "pico/audio_i2s.h"
#include "pico/binary_info.h"
bi_decl(bi_3pins_with_names(PICO_AUDIO_I2S_DATA_PIN, "I2S DIN", PICO_AUDIO_I2S_CLOCK_PIN_BASE, "I2S BCK", PICO_AUDIO_I2S_CLOCK_PIN_BASE + 1, "I2S LRCK"));

uint32_t SAMPLE_RATE = 44100;
#define SAMPLES_PER_BUFFER 32
repeating_timer_t mainTimer;
bool triggerSample = false;
int samplePos = INT16_MAX;
bool squareToggle = false;

// timing...
uint64_t realTime = 0;
uint64_t dacTime = 0;
uint64_t timeLog[1000] = {0};
int sampleLog[1000] = {0};
int timeLogIndex = 0;

// sync...

bool mainTimerLogic(repeating_timer_t *rt)
{
    triggerSample = true;
    squareToggle = !squareToggle;
    gpio_put(17, squareToggle);
    realTime = time_us_64();
    return true;
}

// Borrowed/adapted from pico-playground
struct audio_buffer_pool *init_audio()
{

    static audio_format_t audio_format = {
        SAMPLE_RATE,
        AUDIO_BUFFER_FORMAT_PCM_S16,
        2,
    };

    static struct audio_buffer_format producer_format = {
        .format = &audio_format,
        .sample_stride = 4};

    struct audio_buffer_pool *producer_pool = audio_new_producer_pool(&producer_format, 1,
                                                                      SAMPLES_PER_BUFFER); // todo correct size
    bool __unused ok;
    const struct audio_format *output_format;
    struct audio_i2s_config config = {
        .data_pin = 9,
        .clock_pin_base = 10,
        .dma_channel = 2, // was 0, trying to avoid SD conflict
        .pio_sm = 0,
    };

    output_format = audio_i2s_setup(&audio_format, &config);
    if (!output_format)
    {
        panic("PicoAudio: Unable to open audio device.\n");
    }

    ok = audio_i2s_connect(producer_pool);
    assert(ok);
    audio_i2s_set_enabled(true);
    return producer_pool;
}

// main function, obviously
int main()
{
    stdio_init_all();
    alarm_pool_init_default(); // not sure if needed

    gpio_init(17);
    gpio_set_dir(17, GPIO_OUT);

    printf("Pico Audio template\n");

    struct audio_buffer_pool *ap = init_audio();
    add_repeating_timer_ms(1, mainTimerLogic, NULL, &mainTimer);
    
    // audio buffer loop, runs forever
    while (true)
    {
        struct audio_buffer *buffer = take_audio_buffer(ap, true);
        int16_t *bufferSamples = (int16_t *)buffer->buffer->bytes;

        if(triggerSample) {
            samplePos = 0;
            triggerSample = false;
        }

        if(timeLogIndex<1000) {
            timeLog[timeLogIndex] = time_us_64();
            sampleLog[timeLogIndex] = buffer->max_sample_count;
        }
        timeLogIndex ++;
        if(timeLogIndex == 1000) {
            for(int i=0; i<1000; i++) {
                printf("%llu %d\n", timeLog[i], sampleLog[i]);
            }
        }
        // update audio output (stereo, hence *2 and += 2)
        for (uint i = 0; i < buffer->max_sample_count * 2; i += 2)
        {
            // sample updates go here
            if(samplePos < 441) {
                bufferSamples[i] = 8192; // left
                samplePos ++;
            } else {
                bufferSamples[i] = 0;
            }
            

            bufferSamples[i+1] = 0; // right
            
        }
        buffer->sample_count = buffer->max_sample_count;
        give_audio_buffer(ap, buffer);
    }
    return 0;
}