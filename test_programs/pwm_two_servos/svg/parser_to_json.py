import argparse
import json
import os
import svgpathtools
from svgpathtools import svg2paths

def clamp(value, min_value, max_value):
    """ Clamp the value within the min_value and max_value range. """
    return max(min_value, min(max_value, value))

def normalize_point(point, min_x, max_x, min_y, max_y):
    """ Normalize a point to the range [-1, 1] and clamp the values. """
    x = clamp(-1 + 2 * (point[0] - min_x) / (max_x - min_x), -1, 1)
    y = clamp(-1 + 2 * (point[1] - min_y) / (max_y - min_y), -1, 1)
    return [x, y]

def sample_path(path, distance_between_points, min_x, max_x, min_y, max_y):
    """ Sample points from the path based on the desired distance between points and normalize them. """
    length = path.length()
    num_points = max(2, int(length / distance_between_points))
    return [normalize_point((path.point(t / (num_points - 1)).real, path.point(t / (num_points - 1)).imag), min_x, max_x, min_y, max_y) for t in range(num_points)]

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

def parse_svg(svg_file):
    """ Parse the SVG file and return a list of paths. """
    paths, _ = svg2paths(svg_file)
    return paths

def write_to_json(output_file, all_paths):
    """ Write the paths data to a JSON file. """
    with open(output_file, 'w') as file:
        json.dump(all_paths, file, indent=4)

def main():
    parser = argparse.ArgumentParser(description='Process an SVG file to sample and normalize points from paths.')
    parser.add_argument('svg_file', type=str, help='Path to the SVG file')
    parser.add_argument('--distance', type=float, default=1.0, help='Approximate distance between sampled points')
    parser.add_argument('--output_file', type=str, help='Output JSON file name')
    args = parser.parse_args()

    if not args.output_file:
        base_name = os.path.splitext(args.svg_file)[0]
        args.output_file = f"{base_name}.json"

    paths = parse_svg(args.svg_file)
    min_x, max_x, min_y, max_y = calculate_bounds(paths)
    all_points = [sample_path(path, args.distance, min_x, max_x, min_y, max_y) for path in paths]

    write_to_json(args.output_file, all_points)
    print(f"Output written to {args.output_file}")

if __name__ == "__main__":
    main()

