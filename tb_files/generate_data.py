SIZE_LIST = 10

with open("build/inputs.dat", 'w') as file:

  # random numbers for now

  # list1
  for i in range(SIZE_LIST):
    file.write(str(0.222)+"\n")
    file.write(str(0.222)+"\n")

  # list2
  for i in range(SIZE_LIST):
    file.write(str(i*0.1)+"\n")
    file.write(str(i*0.2)+"\n")
  
  # output
  for i in range(SIZE_LIST):
    file.write(str(i*0.1)+"\n")
    file.write(str(i*0.2)+"\n")
  