# ipk-extractor
A simple program that unpacks files found in the ipk archives from Blinx 2.
Is also capable of repacking files into new ipk archives with additional iph headers.

Unpacks and decompresses all files. Additionally recompresses and repacks files.
        
        Usage: ipk_extract.exe [<infile(s).ipk/ipklog>]

# .ipk files
All ipk files entered will be unpacked and decompressed into corresponding root folders,
each folder sharing a name with their corresponding ipk file.
Additionally, an ipklog file will be produced storing the following information:
    
    Root initializer:
        'START_ROOT_IPK' followed by the ipk file's name in double-quotes
    
    Root header info:
        'ROOT_IPK_SIZE' followed by the size (in bytes) of the entire file
        'ROOT_FOLDER' followed by the absolute path of the extracted ipk files in double-quotes
        'ALIGNMENT' followed by the block alignment (in bytes)
        'AMOUNT_ENTRIES' followed by the number of entries in the ipk file
    
    Entry info:
        Entry ID followed by 'ENTRY_PATH' followed by the relative path and name of the entry in double-quotes
    
    Embedded initializer:
        'START_EMBEDDED_IPK' followed by the embedded ipk file's name in double-quotes
    
    Embedded header info:
        'EMBEDDED_IPK_SIZE' followed by the size (in bytes) of the embedded file
        'ROOT_FOLDER' followed by the absolute path of the embedded files in double-quotes
        'ALIGNMENT' followed by the block alignment (in bytes)
        'AMOUNT_ENTRIES' followed by the number of entries in the embedded ipk file


# .ipklog files
All ipklogs are text files storing information used to create a new ipk and corresponding iph file.
New files will share a name with their corresponding ipklog file.
Each extension will bw appended with '.new' to prevent overwrites.


# (de)compression
Slightly-editted Saxman decompression and compression codes pulled from https://segaretro.org/Saxman_compression because Sega.
