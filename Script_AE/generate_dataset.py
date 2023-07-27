import numpy as np
import argparse
import struct

def process_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("--src", help="The input file (.fvecs)")
    parser.add_argument("--dst", help="The output file (.fvecs)")
    parser.add_argument("--topk", type=int, help="The number of element to pick up")
    return parser.parse_args()


if __name__ == "__main__":
    args = process_args()

    # Read topk vector one by one
    #change from xvec to default
    vecs = ""
    with open(args.src, "rb") as f:

        i = 0
        while 1:

            dim_bin = f.read(4)
            assert dim_bin != b''
            dim, = struct.unpack('i', dim_bin)

            # The next 4 * dim byte is for a vector
            vec = f.read(dim)
            
            # Store it
            vecs += vec
            i += 1
            if i == args.topk:
                break
            
    with open(args.dst, "wb") as f:
        f.write(struct.pack('i', args.topk))
        f.write(dim_bin)
        f.write(vecs)