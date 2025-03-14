#include "esp_log.h"
#include "WAVFileWriter.hpp"

static const char *TAG = "WAV";

// *************************
// Wrapper for C functions
// *************************
extern "C" {
    WAVFileWriter* WAVFileWriter_C(FILE *fp, int sample_rate) {
        return new WAVFileWriter(fp, sample_rate);
    }

    void write_C(WAVFileWriter* writer, int32_t *samples, int count) {
        writer->write(samples, count);
    }

    void finish_C(WAVFileWriter* writer) {
        writer->finish();
    }

    void destroy_C(WAVFileWriter* writer) {
        delete writer;
    }
}


// *************************
// Actual functions
// *************************
WAVFileWriter::WAVFileWriter(FILE *fp, int sample_rate)
{
  m_fp = fp;
  m_header.sample_rate = sample_rate;
  // write out the header - we'll fill in some of the blanks later
  fwrite(&m_header, sizeof(wav_header_t), 1, m_fp);
  m_file_size = sizeof(wav_header_t);
}

void WAVFileWriter::write(int32_t *samples, int count)
{
  // write the samples and keep track of the file size so far
  fwrite(samples, sizeof(int32_t), count, m_fp);
  m_file_size += sizeof(int32_t) * count;
}

void WAVFileWriter::finish()
{
  ESP_LOGI(TAG, "Finishing wav file size: %d", m_file_size);
  // now fill in the header with the correct information and write it again
  m_header.data_bytes = m_file_size - sizeof(wav_header_t);
  m_header.wav_size = m_file_size - 8;
  fseek(m_fp, 0, SEEK_SET);
  fwrite(&m_header, sizeof(wav_header_t), 1, m_fp);
}