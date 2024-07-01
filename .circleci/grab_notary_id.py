import re
import sys

ID_REG = r"id: (.*)"

def main() -> None:
    notary_log = sys.argv[1]
    with open(notary_log, "r") as f:
        notary_output = f.read()
        id_m = re.search(ID_REG, notary_output)
        if id_m:
            print(id_m.group(1))
        else:
            raise RuntimeError("Unable to parse ID from notarization logs")

if __name__ == "__main__":
    main()