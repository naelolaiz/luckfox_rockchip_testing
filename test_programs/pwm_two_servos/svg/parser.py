import argparse
import os
import re
import svgpathtools
from svgpathtools import svg2paths

def sanitize_variable_name(name):
    """ Sanitize the string to be a valid C++ variable name. """
    # Remove invalid characters
    name = re.sub(r'\W|^(?=\d)', '_', name)
    return name

def clamp(value, min_value, max_value):
    """ Clamp the value within the min_value and max_value range. """
    return max(min_value, min(max_value, value))

def normalize_point(point, min_x, max_x, min_y, max_y):
    """ Normalize a point to the range [-1, 1] and clamp the values. """
    x = clamp(-1 + 2 * (point[0] - min_x) / (max_x - min_x), -1, 1)
    y = clamp(-1 + 2 * (point[1] - min_y) / (max_y - min_y), -1, 1)
    return (x, y)

def sample_path(path, num_points, min_x, max_x, min_y, max_y):
    """ Sample num_points equidistant points from the path and normalize them. """
    return [normalize_point((path.point(t / (num_points - 1)).real, path.point(t / (num_points - 1)).imag), min_x, max_x, min_y, max_y) for t in range(num_points)]

def parse_svg(svg_file):
    """ Parse the SVG file and return a list of paths. """
    paths, _ = svg2paths(svg_file)
    return paths

def calculate_bounds(paths):
    """ Calculate the bounding box (min/max x and y) of all points in the paths. """
    min_x = min_y = float('inf')
    max_x = max_y = float('-inf')

    for path in paths:
        for segment in path:
            start_x, start_y = segment.start.real, segment.start.imag
            end_x, end_y = segment.end.real, segment.end.imag
            min_x, max_x = min(min_x, start_x, end_x), max(max_x, start_x, end_x)
            min_y, max_y = min(min_y, start_y, end_y), max(max_y, start_y, end_y)

    return min_x, max_x, min_y, max_y

def write_cpp_files(header_file, cpp_file, variable_name, all_paths):
    """ Write the vector definition and initialization to .h and .cpp files. """
    # Write to the header file
    with open(header_file, 'w') as file:
        guard = os.path.basename(header_file).replace('.', '_').upper()
        file.write(f"#ifndef {guard}\n")
        file.write(f"#define {guard}\n\n")
        file.write("#include <vector>\n\n")
        file.write(f"extern std::vector<std::vector<std::pair<double, double>>> {variable_name};\n")
        file.write(f"#endif // {guard}\n")

    # Write to the cpp file
    with open(cpp_file, 'w') as file:
        file.write(f'#include "{header_file}"\n\n')
        file.write(f"std::vector<std::vector<std::pair<double, double>>> {variable_name} = {{\n")
        for path in all_paths:
            file.write("    {  // Single Path\n")
            for point in path:
                file.write(f"        {{ {point[0]}, {point[1]} }},\n")
            file.write("    },\n")
        file.write("};\n")

def main():
    parser = argparse.ArgumentParser(description='Process an SVG file to sample and normalize points from paths.')
    parser.add_argument('svg_file', type=str, help='Path to the SVG file')
    parser.add_argument('--num_points', type=int, default=30, help='Number of equidistant points to sample from each path')
    parser.add_argument('--output_base', type=str, help='Base name for the output files (without extension)')
    args = parser.parse_args()

    base_name = os.path.splitext(args.svg_file)[0]
    if not args.output_base:
        args.output_base = base_name

    variable_name = sanitize_variable_name(os.path.basename(base_name))
    header_file = f"{args.output_base}.h"
    cpp_file = f"{args.output_base}.cpp"

    paths = parse_svg(args.svg_file)
    min_x, max_x, min_y, max_y = calculate_bounds(paths)
    all_points = [sample_path(path, args.num_points, min_x, max_x, min_y, max_y) for path in paths]

    write_cpp_files(header_file, cpp_file, variable_name, all_points)
    print(f"Output written to {header_file} and {cpp_file}")

if __name__ == "__main__":
    main()

