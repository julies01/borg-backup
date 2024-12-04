import os
import random

# Directory to store generated files
output_dir = "test_files"
os.makedirs(output_dir, exist_ok=True)

# Generate 3 files with at least 10 MB each and containing 10 parts with at least two pairs of the same text in each file
file_size = 200 * 1024  # 40 KB in bytes
part_size = file_size // 10  # Size of each part

# Function to generate part content
def generate_same_part_content(file_index, part_index):
    part_content = []
    # Add pairs of the same text
    pair_text = f"This is a repeated line in file {file_index}, part {part_index}: {random.randint(1000, 9999)}\n"
    while len("".join(part_content).encode('utf-8')) < part_size:
            part_content.append(pair_text)
    return "".join(part_content)

def generate_diff_part_content(file_index, part_index):
    part_content = []
    # Fill the rest of the part with unique lines
    while len("".join(part_content).encode('utf-8')) < part_size:
            text = f"This is a unique line in file {file_index}, part {part_index}: {random.randint(1000, 9999)}\n"
            part_content.append(text)
    return "".join(part_content)

# Generate 3 files
for i in range(1, 4):
    filename = os.path.join(output_dir, f"file_{i}.txt")
    with open(filename, "w") as file:
        # Generate two pairs of identical parts
        part1_content = generate_same_part_content(i, 1)
        part2_content = generate_same_part_content(i, 2)
        part3_content = generate_same_part_content(i, 3)
        part4_content = generate_same_part_content(i, 4)
        
        # Write the parts to the file
        file.write(part1_content)
        file.write(part1_content)  # Same as part1_content
        file.write(part2_content)
        file.write(part2_content)  # Same as part2_content
        file.write(part3_content)
        file.write(part4_content)
        
        # Generate and write the remaining parts
        for j in range(5, 11):
            part_content = generate_diff_part_content(i, j)
            file.write(part_content)

print(f"Generated 3 text files in '{output_dir}' with at least 10 MB each and containing 10 parts with repeated text.")
