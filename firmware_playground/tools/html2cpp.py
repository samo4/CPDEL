import gzip
import re
import argparse
import os

HEADER_TEMPLATE = """#pragma once

#include <cstddef>
#include <pgmspace.h>
#include <stdint.h>

const uint8_t DASH_HTML[] PROGMEM = {{
{array_data}
}};
"""

def extract_compressed_data(file_path):
    with open(file_path, 'r') as file:
        content = file.read()

    match = re.search(r'const uint8_t DASH_HTML\[\d+\] PROGMEM = \{([^}]*)\};', content, re.DOTALL)
    if not match:
        raise ValueError("Compressed data not found in the file")

    data_str = match.group(1)
    data_list = data_str.split(',')
    compressed_data_bytes = bytes(int(x.strip()) for x in data_list if x.strip().isdigit())

    return compressed_data_bytes

def decompress_data(compressed_data_bytes):
    return gzip.decompress(compressed_data_bytes)

def write_to_html(decompressed_data, output_file):
    with open(output_file, 'wb') as file:
        file.write(decompressed_data)

def compress_data(html_data):
    return gzip.compress(html_data)

def write_to_cpp(compressed_data_bytes, output_file):
    #byte_list = ','.join(str(b) for b in compressed_data_bytes)
    #cpp_content = f'#include "dash_webpage.h"\n\nconst uint8_t DASH_HTML[{len(compressed_data_bytes)}] PROGMEM = {{{byte_list}}};'

    array_data = ', '.join(f'0x{byte:02x}' for byte in compressed_data_bytes)

    header_content = HEADER_TEMPLATE.format(
        #array_len=len(binary_data),
        array_data=array_data)

    with open(output_file, 'w') as file:
        file.write(header_content)

def main():
    parser = argparse.ArgumentParser(description='Convert between HTML and C++ compressed data.')
    parser.add_argument('input', help='Input file path')
    parser.add_argument('-o', '--output', help='Output file path', default=None)
    args = parser.parse_args()

    input_file = args.input
    input_extension = os.path.splitext(input_file)[1].lower()
    output_file = args.output

    print(f"Input file: {input_file}")
    print(f"Output file: {output_file}")

    if input_extension == '.cpp':
        output_file = os.path.splitext(input_file)[0] + '.html'
        compressed_data_bytes = extract_compressed_data(input_file)
        decompressed_data = decompress_data(compressed_data_bytes)
        write_to_html(decompressed_data, output_file)
        print(f"Decompressed HTML written to {output_file}")

    elif input_extension == '.html':
        if not output_file:
            output_file = os.path.splitext(input_file)[0] + '_export.h'
        with open(input_file, 'rb') as file:
            html_data = file.read()
        compressed_data_bytes = compress_data(html_data)
        write_to_cpp(compressed_data_bytes, output_file)
        print(f"Compressed HTML written back to {output_file}")

    else:
        print("Unsupported file extension. Please provide a .cpp or .html file.")

if __name__ == "__main__":
    main()
