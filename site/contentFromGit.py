#---------------------------------------------------------------------------
# Populate Pelican content from various markdown files
# Cedric Adjih - Inria - 2014
#---------------------------------------------------------------------------

import os
import subprocess

FileList = [
    # <branch>|None, <path>, (->) <category>, <title>, <fileName>
    (#'master', 'README.branches', 
        None, "../README.branches",
     'main', 'contiki-hiper playground', 'README-main.md')
]

for branch, inFileName, category, title, outFileName in FileList:
    date = "2014-12-10 22:30"
    if branch != None:
        command = "git show %s:%s" % (branch, inFileName)
        fileContent = subprocess.check_output(command.split(" "))
    else:
        f = open(inFileName)
        fileContent = f.read()
        f.close()
    f = open("content/"+outFileName, "w")
    f.write("Title: %s\nDate: %s\nCategory: %s\n\n"
            % (title, date, category))
    f.write(fileContent)
    f.close()

MdFileList = [
    ('sniffer', 'hipercom/sniffer', 'README-sniffer.md'),
    ('sniffer', 'hipercom/sniffer', 'README-record.md'),
    ('sniffer', 'hipercom/sniffer', 'README-internals.md'),
]

for branch, dirName, inFileName in MdFileList:
    command = "git show %s:%s" % (branch, os.path.join(dirName, inFileName))
    fileContent = subprocess.check_output(command.split(" "))
    print fileContent

#---------------------------------------------------------------------------
