#!/usr/bin/env python

import argparse
import re
import os

def replace(infile, outfile):
   SandR = [("LTLSPEC", ""), (":=", ":"), ("NAME", "")]
   with open(infile, "r") as ifile, open(outfile, "w") as ofile:
      for line in ifile:
         if "VAR" not in line and "MODULE" not in line:
            for s, r in SandR:
               line = line.replace(s, r)
               pass
            line = re.sub("[a-zA-Z0-9]+ : [a-zA-Z0-9]+;",
                          "", line, flags = re.M)
            line = re.sub("[ ]*--.*",
                          "", line, flags = re.M)
            line = re.sub("^[ ]+",
                          "", line, flags = re.M)
            if not line.strip(): continue
            #print("+" + line + "*")
            ofile.write(line)
            pass
         pass
      pass

def _is_valid_file(arg):
    """
    Checks whether input file exists
    """
    if not os.path.exists(arg):
        raise argparse.ArgumentTypeError('{} not found!'.format(arg))
    else:
        return arg

def _is_write_file(arg):
    """
    Checks whether input file exists
    """
    if os.path.exists(arg) and not os.access(arg, os.W_OK):
          raise argparse.ArgumentTypeError('{} not writeable!'.format(arg))
    else:
        return arg


def main ():
   DESCRIPTION = """Converter from LTLFUC to AALTA-UC."""
   parser = argparse.ArgumentParser(description = DESCRIPTION,
                                    formatter_class=argparse.ArgumentDefaultsHelpFormatter)

   parser.add_argument('-input', required=True, help='Path to input LTLFUC file', type=_is_valid_file)
   parser.add_argument('-output', required=True, help='Path to output LTLFUC file', type=_is_write_file)

   args = parser.parse_args()

   ifile = args.input
   ofile = args.output
   replace(ifile, ofile)
   pass

if __name__ == '__main__':
    main()
