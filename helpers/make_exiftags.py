with open("EXIFTAGS.txt") as tagfile:
    line = tagfile.readline()
    while line != '':
        line = line.strip()
        arr = line.split('\t')
        print("{" + arr[0] + ", EXIF_IFD_" + arr[1].upper() + "},")
        line = tagfile.readline()
