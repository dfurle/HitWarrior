import pandas as pd
import collections
import matplotlib.pyplot as plt
import numpy as np

with open("data/recoTracks.txt") as file:
  rdata = file.read()

# with open("data/truthTracks.txt") as file:
#   truth = file.read()

rdata = rdata.split("Track: ")[1:]

fdata = []
for track in rdata:
  halved = track.split("---------- sp --------------\n")
  trackinfo = halved[0].split("\n")[1]
  truthinfo = halved[1].split("\n")[-2].split(" ")[-1][:-14]
  hitinfo = halved[1].split("\n")[:-2]
  fdata.append([trackinfo, truthinfo, hitinfo])


# print(fdata[:10])

# rdata = [[d.split("\n")[1], d.split("\n")[-2], d.split("\n")[-7:-2]] for d in rdata]

# CUTTING_FIRST_TRACKS = 2000
# print("Filtering first {} tracks".format(CUTTING_FIRST_TRACKS))

info = collections.defaultdict(list)
# for trackid, track in enumerate(fdata[CUTTING_FIRST_TRACKS:CUTTING_FIRST_TRACKS+200]):
for trackid, track in enumerate(fdata):
  tparts = track[0].split(" ")

  truth = (0 if (track[1] == "18446744073709551615") else 1)
  prob = float(tparts[20])
  if truth == 1 and prob < 0.9:
    # print("filtering by prob: {}".format(trackid))
    truth = 0
  nnscore = float(tparts[17])
  # if nnscore < 0.25:
  #   continue

  # print(truth)
  for hitid in range(5):
    # print("len:",len(track[2]))
    # print("hitid:",hitid)
    if hitid < len(track[2]):
      hit = track[2][hitid]
      hit = hit[:-1]
      parts = hit.split(" ")
    else:
      parts = np.zeros(13)
    # print(hit)
    info["trackid"].append(trackid)
    info["hitid"].append(hitid)
    info["pt"].append(float(tparts[4]))
    # info["prob"].append(float(tparts[10]))
    # info["cprob"].append(prob)
    info["truth"].append(truth)
    info["nnscore"].append(nnscore)

    info["x"].append(float(parts[5]))
    info["y"].append(float(parts[7]))
    info["z"].append(float(parts[9]))
    # hitinfo["rho"] = parts[11]
    # hitinfo["r"] = parts[13]
    # hitlist.append(hitinfo)
    
  
data = pd.DataFrame(info)
# data

THRESHOLD = 0.5
total_length = len(data)
total_truth= len(data[data["truth"] == 1])
total_threshold = len(data[data["nnscore"] > THRESHOLD])
total_true_thresholded = len(data[(data["nnscore"] > THRESHOLD) & (data["truth"] == 1)])

print("total length:           {}".format(total_length))
print("total truth:            {}".format(total_truth))
print("total threshold:        {}".format(total_threshold))
print("total true thresholded: {}".format(total_true_thresholded))
print()
print("percent truth:       {:.2f} %".format(100*total_truth/total_length))
print("percent threshold:   {:.2f} %".format(100*total_threshold/total_length))
print("percent true trehs:  {:.2f} %".format(100*total_true_thresholded/total_length))


data.to_csv("data/formatted_data.csv", index=False, float_format='%.15f')

# plt.hist(data["truth"], bins=2)
# plt.show()

# plt.hist(data["nnscore"], bins=10)
# plt.show()

# plt.hist(data["pt"], bins=10)
# plt.show()

a = len(data.groupby(["trackid"]))
b = len(data[data["truth"] == 1].groupby(["trackid"]))
print("number of true tracks:",a)
print("number of total tracks:",b)
print("percent of true tracks:",b/a*100)

print(data[data["truth"] == 1]["trackid"].unique())