#pragma once

#include <stdio.h>
#include "WAVFile.hpp"
#include "stdint.h"

#ifdef __cplusplus
extern "C++" {

class WAVFileWriter
{
private:
  int m_file_size;

  FILE *m_fp;
  wav_header_t m_header;

public:
  WAVFileWriter(FILE *fp, int sample_rate);
  void start();
  void write(int32_t *samples, int count);
  void finish();
};

} // extern "C++"
#endif


// Add C-compatible wrapper functions
#ifdef __cplusplus
extern "C" {
#endif

  typedef struct WAVFileWriter WAVFileWriter; // Forward Declaration (to ensure compatibility in C files)

  // C-compatible functions for managing WAVFileWriter
  WAVFileWriter* WAVFileWriter_C(FILE *fp, int sample_rate);
  void write_C(WAVFileWriter* writer, int32_t *samples, int count);
  void finish_C(WAVFileWriter* writer);
  void destroy_C(WAVFileWriter* writer);

#ifdef __cplusplus
}
#endif