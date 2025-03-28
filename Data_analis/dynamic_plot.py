import serial
import numpy as np
import matplotlib.pyplot as plt

ser = serial.Serial("/dev/ttyACM0", 9600)


DATA_RANGE = 300
y1 = np.zeros(DATA_RANGE)
y1.fill(np.nan)

yx = np.zeros(DATA_RANGE)
yx.fill(np.nan)

y0 = np.zeros(DATA_RANGE)
y0.fill(np.nan)

x = np.arange(0, DATA_RANGE)

plt.figure(figsize=(10, 6))
graph = plt.plot(x,y1)[0]
plt.pause(0.01)

index = 0
trig = 0

import datetime




def write2file(line):
     with open('data_time.txt','a') as f:
          f.write(line)

while True:
     cc=ser.readline().decode('ascii')
     write2file(cc)
     now = str(datetime.datetime.now())
     write2file(now)



     index += 1
     if index == DATA_RANGE:
         index = 0
         trig = 1

     if(cc[0:2] == 'A0'):
          A0 = int(cc[4:])
          write2file(f'A0 {A0}\n')

          if trig == 1:
               x = x+1
               y0[:-1] = y0[1:]
               y0[DATA_RANGE-1] = A0
          else:
               y0[index] = A0


     if(cc[0:2] == 'Ax'):
          Ax = int(cc[4:])
          write2file(f'Ax {Ax}\n')

          if trig == 1:
               x = x+1
               yx[:-1] = y[1:]
               yx[DATA_RANGE-1] = A1
          else:
               yx[index] = Ax

     if(cc[0:2] == 'A1'):

          A1 = int(cc[4:])
          write2file(f'A1 {A1}\n')

          if trig == 1:
               x = x+1
               y1[:-1] = y1[1:]
               y1[DATA_RANGE-1] = A1
          else:
               y1[index] = A1


     graph.remove()
     graph = plt.scatter(x,y1,color = 'r')
     graph = plt.scatter(x,yx,color = 'g')
     graph = plt.scatter(x,y0,color = 'b')

     

     # plt.ylabel('Napięcie U [uV]')
     # plt.xlabel('Ilość Obrotów')
     plt.grid(color='k', linestyle='-', linewidth=0.1)
     plt.xlim(x[0], x[-1])

     plt.pause(0.01)
