import subprocess
import re
import sys

exe_single   = "./sharpen"
exe_multi    = "./sharpen_grid"
infile_small = "Cactus-120kpixel.ppm"
infile_big   = "Cactus-12mpixel.ppm"
outfile      = "out.ppm"
src_single   = "sharpen.c"
src_multi    = "sharpen_grid.c"

img_height_str   = "#define IMG_HEIGHT"
img_width_str    = "#define IMG_WIDTH"
small_img_width  = 400
small_img_height = 300
big_img_width    = 4000
big_img_height   = 3000

num_col_threads_str = "#define NUM_COL_THREADS"
num_row_threads_str = "#define NUM_ROW_THREADS"

float_regex = r"[-+]?(?:\d*\.\d+|\d+)"

def build():
    subprocess.run(["make"], stdout=subprocess.PIPE, stderr=subprocess.PIPE)

def clean():
    subprocess.run(["make", "clean"], stdout=subprocess.PIPE)

def set_value(file, string, value):
    data = ""
    with open(file, "r") as fp:
        for line in fp:
            if line.find(string) == 0:
                data += string + " " + str(value) + "\n"
            else:
                data += line

    with open(file, "w") as fp:
        fp.write(data)

def run(exe, in_img):
    results = subprocess.run([exe, in_img , outfile], stdout=subprocess.PIPE)
    results = results.stdout.decode("utf-8")

    return  list(map(float, re.findall(float_regex, results)))

def usage():
    print("usage: python3 test.py <1,2,3>")
    exit(1)

def case1():
    print("###############################################################################")
    print("# Case 1) single threaded 120k image vs 12M image")
    print("###############################################################################")

    read_times = []
    exe_times  = []
    print("Using 120K image")
    clean()
    set_value(src_single,img_height_str,small_img_height)
    set_value(src_single,img_width_str,small_img_width)
    build()

    for i in range(0,10):
        print("Running Iteration: " + str(i))
        nums = run(exe_single, infile_small)
        read_times.append(nums[0])
        exe_times.append(nums[1] - nums[0])
    print(exe_times)
    print(read_times)
    print()

    read_times = []
    exe_times  = []
    print("Using 12M image")
    clean()
    set_value(src_single,img_height_str,big_img_height)
    set_value(src_single,img_width_str,big_img_width)
    build()

    for i in range(0,10):
        print("Running Iteration: " + str(i))
        nums = run(exe_single, infile_big)
        read_times.append(nums[0])
        exe_times.append(nums[1] - nums[0])
    print(exe_times)
    print(read_times)
    print()

    clean()

def case2():
    print("###############################################################################")
    print("# Case 2) 3 Row Threads 4 Col Threads. 120K vs 12M")
    print("###############################################################################")

    exe = exe_multi
    src = src_multi
    read_times = []
    exe_times  = []

    print("Using 120K image")
    clean()
    set_value(src, img_height_str,small_img_height)
    set_value(src, img_width_str,small_img_width)
    set_value(src, num_row_threads_str, 3)
    set_value(src, num_col_threads_str, 4)
    build()

    for i in range(0,10):
        print("Running Iteration: " + str(i))
        nums = run(exe, infile_small)
        print(nums)
        read_times.append(nums[0])
        exe_times.append(nums[1] - nums[0])
    print(exe_times)
    print(read_times)
    print()

    read_times = []
    exe_times  = []
    print("Using 12M image")
    clean()
    set_value(src,img_height_str,big_img_height)
    set_value(src,img_width_str,big_img_width)
    build()

    for i in range(0,10):
        print("Running Iteration: " + str(i))
        nums = run(exe, infile_big)
        read_times.append(nums[0])
        exe_times.append(nums[1] - nums[0])
    print(exe_times)
    print(read_times)
    print()

    clean()



###############################################################################
# main
###############################################################################

if len(sys.argv) != 2:
    usage()

case = int(sys.argv[1])
if not case in [1,2]:
    usage()

if case == 1:
    case1()
    exit(0)

if case == 2:
    case2()
    exit(0)
