def read_ppm(filename):
    with open(filename, 'r') as f:
        assert f.readline().strip() == 'P3'  # Check header
        width, height = map(int, f.readline().split())
        max_val = int(f.readline())
        pixels = []

        for _ in range(height):
            row = []
            for _ in range(width):
                r = int(f.readline())
                g = int(f.readline())
                b = int(f.readline())
                row.append((r, g, b))
            pixels.append(row)

        return width, height, max_val, pixels


def write_ppm(filename, width, height, max_val, pixels):
    with open(filename, 'w') as f:
        f.write("P3\n")
        f.write(f"{width} {height}\n")
        f.write(f"{max_val}\n")
        for row in pixels:
            for r, g, b in row:
                f.write(f"{r}\n{g}\n{b}\n")


def box_blur(pixels, width, height):
    from copy import deepcopy
    original = deepcopy(pixels)
    result = [[(0, 0, 0) for _ in range(width)] for _ in range(height)]

    for y in range(height):
        for x in range(width):
            r_sum = g_sum = b_sum = count = 0
            for dy in [-1, 0, 1]:
                for dx in [-1, 0, 1]:
                    ny, nx = y + dy, x + dx
                    if 0 <= ny < height and 0 <= nx < width:
                        r, g, b = original[ny][nx]
                        r_sum += r
                        g_sum += g
                        b_sum += b
                        count += 1
            result[y][x] = (r_sum // count, g_sum // count, b_sum // count)

    return result


if __name__ == "__main__":
    input_file = "input.ppm"
    output_file = "output_blurred.ppm"

    width, height, max_val, pixels = read_ppm(input_file)
    blurred = box_blur(pixels, width, height)
    write_ppm(output_file, width, height, max_val, blurred)
    print(f"Saved blurred image as {output_file}")