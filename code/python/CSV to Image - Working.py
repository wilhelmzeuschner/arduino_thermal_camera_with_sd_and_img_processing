#!/usr/local/bin/python3
#https://stackoverflow.com/questions/52027382/how-do-i-convert-a-csv-file-16bit-high-color-to-image-in-python
#pip install numpy
#pip install pillow


import numpy as np

from tkinter import *
from tkinter import filedialog
from PIL import Image



top = Tk()

top.minsize(300, 210)
top.maxsize(300, 210)
top.title("Thermal Camera Image Processor")


file_selected = 0
file_name = ""
save_selected = 0
save_name = ""

def process_image():
    global file_name
    global file_selected
    processed = 0
    if file_selected != 0:
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
                        rgb565 = rgb565array[row,col]
                        r = ((rgb565 >> 11 ) & 0x1f ) << 3
                        g = ((rgb565 >> 5  ) & 0x3f ) << 2
                        b = ((rgb565       ) & 0x1f ) << 3
                        # Populate result array
                        rgb888array[row,col]=r,g,b

                # Save result as PNG
                Image.fromarray(rgb888array).save("thermal_image.png")
                info2["text"]= str("Saved file to: " + str("thermal_image.png"))
            except:
                print("There was an error while processing, opening or saving the file! Make sure that is has the correct format!")
    else:
        print("No File selected!")
        
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
            info2["text"]= str("Saved file to: " + str(str(save_name) + '.png'))

        except:
                print("There was an error while processing, opening or saving the file! Make sure that is has the correct format!")
    elif processed == 0:
        print("No File selected!")

def select_file():
    global file_name
    global file_selected
    print("Selecting File")
    top.filename =  filedialog.askopenfilename(initialdir = "/",title = "Select file",filetypes = (("txt files","*.txt"),("csv files","*.csv*"),("all files","*.*")))
    if top.filename != "":
        print (top.filename)
        file_selected = 1
        file_name = top.filename
        file_path.delete(0, "end")
        file_path.insert(0, str(file_name))


def set_file():
    global save_name
    global save_selected
    print("Selecting File")
    top.savename =  filedialog.asksaveasfilename(initialdir = "/",title = "Save as",filetypes = (("jpeg files","*.jpg"),("all files","*.*")))
    if top.savename != "":
        print (save_name)
        save_name = top.savename
        save_selected = 1
        save_path.delete(0, "end")
        save_path.insert(0, str(save_name))

select = Button(top, text= "Select File", command=select_file, background= "green")
save_to = Button(top, text= "Save as", command=set_file, background= "yellow")
process = Button(top,text= "Process selected File", command=process_image, background= "orange")
file_path = Entry(top)
save_path = Entry(top)

info = Label(top, text = "If you don't select a path \n where the image should be saved,\n it will be saved in the directory \n where this .py file is run from. \n Previous files that have been saved with \n this method will be overwritten!")
info2 = Label(top, text = "")

select.grid(row = 1,column = 1)
save_to.grid(row = 2,column = 1)

process.grid(row = 3,column = 0)


file_path.grid(row= 1, column= 0)
save_path.grid(row= 2, column= 0)

info.grid(row= 4, column=0)
info2.grid(row= 5, column=0)


top.mainloop()
