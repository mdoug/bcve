import sys
import subprocess

from tkinter import *
from tkinter import ttk
from tkinter import filedialog

system='WINDOWS'

root = Tk()
root.title("BCVE Launcer")

exe_file  = ""

file_text = StringVar()
dir_text  = StringVar()
make_log  = StringVar()
use_defaults = StringVar()
disp_1080p_full = StringVar()
width_value = StringVar()
height_value = StringVar()
f_toggle_value = StringVar()

if (len(sys.argv) > 1):
    file_text.set(sys.argv[1])

def get_dir_from_filepath(filepath):
    tmp_len = len(filepath)
    last_slash = 0
    while tmp_len > 0:
        tmp_len = tmp_len - 1
        if filepath[tmp_len] == '\\' or filepath[tmp_len] == '/':
            last_slash = tmp_len + 1
            break
    return filepath[:last_slash]

def get_full_exe_path():
    return get_dir_from_filepath(sys.argv[0])

if system == 'WINDOWS':
    exe_path = get_full_exe_path()
    exe_file = exe_path + "bcve"

def set_file ():
#    start_dir = get_dir_from_filepath(file_text.get())

    start_dir = file_text.get()

    file_text.set(filedialog.askopenfilename(initialfile = start_dir))

def set_dir ():
    print ("file_text.get():", file_text.get())
    start_dir = get_dir_from_filepath(file_text.get())
    print ("start_dir:", start_dir)
#   start_dir = file_text.get()
    file_text.set(filedialog.askdirectory(initialdir = start_dir, mustexist = True))

def launch():
    args = [exe_file]
    if use_defaults.get() == "no":
        if disp_1080p_full.get() == "yes":
            width = "1920"
            height = "1080"
            fullscreen = True
        else:
            width = width_value.get()
            height = height_value.get()
            fullscreen = False
        args.append("-w" + width)
        args.append("-h" + height)
        if fullscreen == True :
            args.append("-F")
    if make_log.get() == "yes":
        args.append("-L")
    args.append(file_text.get())
    subprocess.Popen(args)

def default_check():
    if use_defaults.get() == "yes":
        disp_1080p_full.set("no")
        h_field.state(['disabled'])
        w_field.state(['disabled'])
        f_check.state(['disabled'])
    else:
        h_field.state(['!disabled'])
        w_field.state(['!disabled'])
        f_check.state(['!disabled'])

def disp_1080p_full_check():
    if disp_1080p_full.get() == "yes":
        use_defaults.set("no")
        h_field.state(['disabled'])
        w_field.state(['disabled'])
        f_check.state(['disabled'])
    else:
        h_field.state(['!disabled'])
        w_field.state(['!disabled'])
        f_check.state(['!disabled'])

mainframe = ttk.Frame(root);
mainframe.grid(column=0, row=0, sticky=(N, W, E, S));

file_entry = ttk.Entry(mainframe, width=50, textvariable=file_text)
file_entry.grid(column=1, row=1, columnspan = 3, sticky=(W))
ttk.Button(mainframe, text="Open File", command=set_file).grid(column=4, row=1,
                                                               sticky=W)
ttk.Button(mainframe, text="Open Dir", command=set_dir).grid(column=4, row = 2, sticky=W)

make_log.set("no")
ttk.Checkbutton(mainframe, text="Make Log?", variable=make_log,
                onvalue="yes", offvalue="no").grid(column=1, row=2, sticky=W)

use_defaults.set("yes")
ttk.Checkbutton(mainframe, text="Use Defaults?", command=default_check,
                variable=use_defaults,
                onvalue="yes", offvalue="no").grid(column=1, row=3, sticky=W)
ttk.Checkbutton(mainframe, text="1080p, Fullscreen?", command=disp_1080p_full_check,
                variable=disp_1080p_full,
                onvalue="yes", offvalue="no").grid(column=1, row=4, sticky=W)

custom_start = 5;
ct = custom_start;

ttk.Label(mainframe, text = "Width").grid(column = 1, row=ct, sticky=W)
w_field = ttk.Entry(mainframe, width=4, textvariable=width_value)
w_field.grid(column=1, row=ct)
w_field.state(['disabled'])

ttk.Label(mainframe, text = "Heigth").grid(column = 1, row=ct + 1, sticky=W)
h_field = ttk.Entry(mainframe, width=4, textvariable=height_value)
h_field.grid(column=1, row=ct + 1)
h_field.state(['disabled'])

f_toggle_value.set("no")
f_check = ttk.Checkbutton(mainframe, text="Fullscreen", onvalue="yes", offvalue="no",
                variable=f_toggle_value)
f_check.grid(column = 1, row = ct + 2, sticky = W)
f_check.state(['disabled'])

ttk.Button(mainframe, text="Launch BCVE", command=launch).grid(column=4,
                                                               row=ct + 3, sticky=E)

root.mainloop()
