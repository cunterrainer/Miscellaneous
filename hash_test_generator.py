import random
import string
import hashlib

def generate_random_string(max_length):
    length = random.randint(0, max_length)
    characters = (string.ascii_letters + string.digits + string.punctuation + " ").replace("@", "")
    random_string = ''.join(random.choice(characters) for _ in range(length))
    return random_string

def calculate_hashes(input_string):
    md5_hash = hashlib.md5(input_string.encode()).hexdigest()
    sha1_hash = hashlib.sha1(input_string.encode()).hexdigest()
    sha224_hash = hashlib.sha224(input_string.encode()).hexdigest()
    sha256_hash = hashlib.sha256(input_string.encode()).hexdigest()
    sha384_hash = hashlib.sha384(input_string.encode()).hexdigest()
    sha512_hash = hashlib.sha512(input_string.encode()).hexdigest()
    sha3_224_hash = hashlib.sha3_224(input_string.encode()).hexdigest()
    sha3_256_hash = hashlib.sha3_256(input_string.encode()).hexdigest()
    sha3_384_hash = hashlib.sha3_384(input_string.encode()).hexdigest()
    sha3_512_hash = hashlib.sha3_512(input_string.encode()).hexdigest()

    return (
        md5_hash, sha1_hash, sha224_hash, sha256_hash,
        sha384_hash, sha512_hash,
        sha3_224_hash, sha3_256_hash, sha3_384_hash, sha3_512_hash
    )

# Generate a list of 1 random string with a length up to 100
random_strings = [generate_random_string(1000) for _ in range(20)]

# Print each string along with its hash values
for random_string in random_strings:
    hashes = calculate_hashes(random_string)
    random_string = random_string.replace("\\", "\\\\").replace("\"", "\\\"")
    print(f'{{\n\t"{random_string}",')
    for algo, hash_value in zip(
        ['MD5', 'SHA-1', 'SHA-224', 'SHA-256', 'SHA-384', 'SHA-512',
         'SHA3-224', 'SHA3-256', 'SHA3-384', 'SHA3-512'],
        hashes
    ):
        print(f'\t"{hash_value}",')
    print("},\n\n")