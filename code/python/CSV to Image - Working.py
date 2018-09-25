#Wilhelm Zeuschner
#Version 2.0
#05.09.2018
#!/usr/local/bin/python3
#https://stackoverflow.com/questions/52027382/how-do-i-convert-a-csv-file-16bit-high-color-to-image-in-python

#If you get errors running this file, make sure to install Python 3 and run these commands in the command prompt or command line (admin mode / sudo)
#pip install numpy
#pip install pillow


import numpy as np

from tkinter import *
from tkinter import filedialog
from PIL import Image
import os



top = Tk()

top.minsize(270, 165)
top.maxsize(270, 165)
top.title("Thermal Camera Image Processor")


file_selected = 0
file_name = ""
save_selected = 0
save_name = ""

last_save  = ""

def process_image():
    global file_name
    global file_selected
    global last_save
    processed = 0
    if file_selected != 0:
        #If a save file name was not specified
        if save_selected == 0 or save_path.get() == "":
            processed = 1
            # Read 16-bit RGB565 image into array of uint16
            try:
                with open(file_name,'r') as f:
                    rgb565array = np.genfromtxt(f, delimiter = ',').astype(np.uint16)

                # Pick up image dimensions
                h, w = rgb565array.shape

                # Make a numpy array of matching shape, but allowing for 8-bit/channel for R, G and B
                rgb888array = np.zeros([h,w,3], dtype=np.uint8)

                for row in range(h):
                    for col in range(w):
                        # Pick up rgb565 value and split into rgb888
                        rgb565 = rgb565array[col,row]
                        r = ((rgb565 >> 11 ) & 0x1f ) << 3
                        g = ((rgb565 >> 5  ) & 0x3f ) << 2
                        b = ((rgb565       ) & 0x1f ) << 3
                        # Populate result array
                        rgb888array[col,row]=r,g,b

                # Save result as PNG
                Image.fromarray(rgb888array).save(f.name[0:-4] + '.png')
                last_save = f.name[0:-4] + '.png'
                info2["fg"] = "black"
                info2["text"]= str("Saved file to: " + str(f.name[0:-4] + '.png'))
                #Now rotate the created image by 90 degrees counter-clockwise
                #This is necessary due to a flaw / the way the image data is saved to the SD card
                im1 = Image.open(str(f.name[0:-4] + '.png'))
                im1 = im1.rotate(90)
                im1.save(str(f.name[0:-4] + '.png'))
                open_file_bt.configure(state = "normal")
            except:
                print("There was an error while processing, opening or saving the file! Make sure that is has the correct format!")
                info2["fg"] = "red"
                info2["text"] = "Error while processing, opening or saving the file!"
                open_file_bt.configure(state = "disabled")
    else:
        print("No File selected!")
        info2["fg"] = "red"
        info2["text"] = "No File selected!"
        open_file_bt.configure(state = "disabled")
        
    if file_selected != 0 and save_selected != 0 :
        try:
        # Read 16-bit RGB565 image into array of uint16
            with open(file_name,'r') as f:
                rgb565array = np.genfromtxt(f, delimiter = ',').astype(np.uint16)

            # Pick up image dimensions
            h, w = rgb565array.shape

            # Make a numpy array of matching shape, but allowing for 8-bit/channel for R, G and B
            rgb888array = np.zeros([h,w,3], dtype=np.uint8)

            for row in range(h):
                for col in range(w):
                    # Pick up rgb565 value and split into rgb888
                    rgb565 = rgb565array[row,col]
                    r = ((rgb565 >> 11 ) & 0x1f ) << 3
                    g = ((rgb565 >> 5  ) & 0x3f ) << 2
                    b = ((rgb565       ) & 0x1f ) << 3
                    # Populate result array
                    rgb888array[row,col]=r,g,b

            # Save result as PNG
            Image.fromarray(rgb888array).save(str(save_name) + '.png')
            info2["fg"] = "black"
            info2["text"]= str("Saved file to: " + str(str(save_name) + '.png'))
            last_save = str(str(save_name) + '.png')
            #Now rotate the created image by 90 degrees counter-clockwise
            #This is necessary due to a flaw / the way the image data is saved to the SD card
            im1 = Image.open(str(str(save_name) + '.png'))
            im1 = im1.rotate(90)
            im1.save(str(str(save_name) + '.png'))
            open_file_bt.configure(state = "normal")

        except:
                print("There was an error while processing, opening or saving the file! Make sure that is has the correct format!")
                info2["fg"] = "red"
                info2["text"] = "Error while processing, opening or saving the file!"
                open_file_bt.configure(state = "disabled")
    elif processed == 0:
        print("No File selected!")
        info2["fg"] = "red"
        info2["text"] = "No File selected!"
        open_file_bt.configure(state = "disabled")

def select_file():
    global file_name
    global file_selected
    print("Selecting File")
    top.filename =  filedialog.askopenfilename(initialdir = "F:",title = "Select file",filetypes = (("txt files","*.txt"),("csv files","*.csv*"),("all files","*.*")))
    if top.filename != "":
        print (top.filename)
        file_selected = 1
        file_name = top.filename
        file_path.delete(0, "end")
        file_path.insert(0, str(file_name))

def open_img():
    global last_save
    print("Opening file")
    try:
        os.startfile(last_save)
    except:
        info2["fg"] = "red"
        info2["text"] = "Could not open image!"


def set_save_path():
    global save_name
    global save_selected
    print("Selecting File")
    top.savename =  filedialog.asksaveasfilename(initialdir = "F:",title = "Save as",filetypes = (("png files","*.png"), ("jpeg files","*.jpeg"),("all files","*.*")))
    if top.savename != "":
        print (save_name)
        save_name = top.savename
        save_selected = 1
        save_path.delete(0, "end")
        save_path.insert(0, str(save_name))

select = Button(top, text= "Select File", command=select_file, background= "green")
save_to = Button(top, text= "Save as", command=set_save_path, background= "yellow")
process = Button(top,text= "Process selected File", command=process_image, background= "orange")
open_file_bt = Button(top, text= "Open Image", command=open_img, background= "RoyalBlue1")

file_path = Entry(top, width = 30)
save_path = Entry(top, width = 30)

info = Label(top, text = "If you don't select a path \n where the image should be saved,\n it will be saved in the directory \n where this .py file is run from.")
info2 = Label(top, text = "")

select.grid(row = 1,column = 1, sticky=E)
save_to.grid(row = 2,column = 1, sticky=E)

open_file_bt.grid(row = 3,column = 1, sticky=E)

process.grid(row = 3,column = 0)


file_path.grid(row= 1, column= 0, columnspan = 2, sticky=W)
save_path.grid(row= 2, column= 0, columnspan = 2, sticky=W)

info.grid(row= 4, column=0)
info2.grid(row= 5, column=0, columnspan = 3, sticky = "W")


open_file_bt.configure(state = "disabled")

top.mainloop()
