# Define the file path where you want to save the file
file_path = 'medium_nonsense_file.txt'

# Create a block of nonsense text
nonsense_text = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. " * 1000  # A single block of text
size_gb = 1 * 512 * 512 * 512  # 1 GB in bytes

# Open the file and write until the desired size is reached
with open(file_path, 'w') as f:
    while f.tell() < size_gb:
        f.write(nonsense_text)

print(f"Generated a large text file: {file_path}")
