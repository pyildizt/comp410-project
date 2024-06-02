import sys

def process_text(text):
    # Split the text into lines
    lines = text.strip().split('\n')
    
    # Remove the first line and the last two lines
    if len(lines) > 3:
        lines = lines[1:-2]
    else:
        return "Insufficient lines to process after removal."

    # Dictionary to store the first letter of each line and its count
    first_letter_count = {}

    # Process each line
    for line in lines:
        if line:  # Check if the line is not empty
            first_letter = line[0]
            if first_letter in first_letter_count:
                first_letter_count[first_letter] += 1
            else:
                first_letter_count[first_letter] = 1

    return first_letter_count

# Read text from standard input
input_text = sys.stdin.read()

# Process the text and print the results
result = process_text(input_text)
print(result)

