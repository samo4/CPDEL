#!/usr/bin/env python3
## convert_files_to_headers.py
## This converts all files in a directory passed as an argument to a header files in include directory
## under the same directory as the original files. The files are also gzipped to save space.
## Usage: python3 convert_files_to_headers.py <directory_path>

import os
import sys
import gzip
import mimetypes
from io import BytesIO

HEADER_TEMPLATE = """#pragma once

#include <cstddef>
#include <pgmspace.h>
#include <stdint.h>

constexpr size_t {array_name}_gz_len = {array_len};
constexpr char {array_name}_content_type[] = "{content_type}";

const uint8_t {array_name}_gz[] PROGMEM = {{
{array_data}
}};
"""

def file_to_header(input_file, output_file, array_name):
    with open(input_file, 'rb') as f_in:
        file_data = f_in.read()

    gzipped_data = BytesIO()
    with gzip.GzipFile(fileobj=gzipped_data, mode='wb') as f_out:
        f_out.write(file_data)

    binary_data = gzipped_data.getvalue()

    array_data = ', '.join(f'0x{byte:02x}' for byte in binary_data)

    content_type, _ = mimetypes.guess_type(input_file)
    if content_type is None:
        content_type = 'application/octet-stream'

    header_content = HEADER_TEMPLATE.format(
        array_name=array_name,
        array_len=len(binary_data),
        content_type=content_type,
        array_data=array_data)

    with open(output_file, 'w') as f:
        f.write(header_content)

def convert_directory(directory_path):
    include_dir = os.path.join(directory_path, 'include')
    os.makedirs(include_dir, exist_ok=True)

    for filename in os.listdir(directory_path):
        input_file = os.path.join(directory_path, filename)
        if os.path.isfile(input_file):
            array_name = os.path.splitext(filename)[0].replace('-', '_').replace('.', '_')
            output_file = os.path.join(include_dir, f'{array_name}.h')
            file_to_header(input_file, output_file, array_name)
            print(f'Converted {input_file} to {output_file}')

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python3 convert_files_to_headers.py <directory_path>")
        sys.exit(1)

    directory_path = sys.argv[1]

    if not os.path.isdir(directory_path):
        print(f"Error: {directory_path} is not a directory.")
        sys.exit(1)

    convert_directory(directory_path)
    print("Conversion completed successfully.")
