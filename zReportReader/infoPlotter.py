import pandas as pd
import matplotlib.pyplot as plt
from matplotlib.axes import Axes
from scipy.optimize import curve_fit
import glob
import numpy as np
import os

def exp(x, A, b):
  return A * np.exp(x * b)

def quad(x, a, b, c):
  return a * x**2 + b * x + c

def linear(x, a, b):
  return a * x + b

class FitFunc:
  def __init__(self, f_str, precision=3, r_precision=4):
    self.p0 = None
    if isinstance(f_str, list):
      self.p0 = f_str[1]
      f_str = f_str[0]

    self.name = f_str
    precision = "{{:.{}f}}".format(precision)
    r_precision = "{{:.{}f}}".format(r_precision)
    fmt = {"p":precision, "r":r_precision}

    if f_str == 'linear':
      self.f = linear
      self.format_string = "{p}x+{p} R2={r}".format(**fmt)
    elif f_str == 'quad':
      self.f = quad
      self.format_string = "{p}*x^2+{p}x+{p} R2={r}".format(**fmt)
    elif f_str == 'exp':
      self.f = exp
      self.format_string = "{p}*exp({p}x) R2={r}".format(**fmt)
      if self.p0 is None: # default to this so it has a possibility to fit it
        self.p0 = [1,0]
    else:
      print("Cannot find function {}, please define it in infoPlotter.py and add to FitFunc check".format(f_str))

  def to_string(self, *pars):
    return self.format_string.format(*pars)
  
class PlotInfo:
  def __init__(self):
    self.colors: list = []
    self.fits: list = []
    self.selections: list = []
    self.final_call: function = None
    self.ax: Axes = None
    self.ylims: list = None


class TrackVSPlot:
  '''
  Initialize with a path
  - call set_modules(<modules>)
  - setup_*() to initialize various parameters and colors for this plot
  - plot()
  !!! can do dot call, TrackVSPlot().setup().setup().plot() !!
  '''
  def __init__(self, path:str, modules:list, ID:int, default_color="blue", print_debug:bool=False):
    self.path: str = path
    self.default_color = default_color
    self.track_sizes = []
    for f in sorted(glob.glob(path + "*TS.csv")):
      f = os.path.basename(f)
      f = os.path.splitext(f)[0]
      f = f[:-2]
      self.track_sizes.append(int(f))
    self.track_sizes = np.array(sorted(self.track_sizes))

    self.modules: list = modules

    self.info: dict = {}
    self.info["resources"] = PlotInfo()
    self.info["latency"] = PlotInfo()
    self.info["compile"] = PlotInfo()

    self.ID = ID
    self.debug: bool = print_debug

    self._read_data() # maybe give call to user ? 

  def printdb(self, str: str):  
    if self.debug:
      print(str)

  def _read_data(self, suffix="TS.csv"):
    data = pd.DataFrame()

    takeHeaders = ["Modules & Loops", "Latency (cycles)", "Latency (ns)", "Track Size", "Timing (min)"]
    # takeHeaders += extraHeaders # TODO: may be useful, add later
    resource_headers = ["BRAM", "DSP", "FF", "LUT", "URAM"]
    takeHeaders += resource_headers
    takeHeaders += [r + " %" for r in resource_headers]

    # file_names = [self.path + str(s) + suffix for s in self.track_sizes]
    file_names = sorted(glob.glob(self.path + "*.csv"))

    for f in file_names:
      if(not os.path.exists(f)):
        self.printdb("Failed to find file {}".format(f))
        continue
      self.printdb("Reading file {}".format(f))
      rdata = pd.read_csv(f)
      trimmed_data = rdata[rdata["Modules & Loops"].isin(self.modules)][takeHeaders].reset_index(drop=True)

      if self.debug:
        for m in self.modules:
          if not rdata["Modules & Loops"].isin([m]).any():
            self.printdb(" Module {} not found, skipping...".format(m))

      data = pd.concat([data, trimmed_data])

    data = data.sort_values("Track Size", ascending=True) # sort by track size

    self.data = data


  # def set_modules(self, modules: list):
  #   self.modules = modules

  def _setup(self, name, colors, fits, ax, final_call, selections, ylims):
    if name not in self.info:
      self.info[name] = PlotInfo()

    if colors:
      self.info[name].colors = colors
    elif selections:
      self.info[name].colors = [self.default_color] * len(selections)

    if fits:
      self.info[name].fits = [FitFunc(f, 3, 5) for f in fits]

    if final_call:
      self.info[name].final_call = final_call

    if ax:
      self.info[name].ax = ax

    if ylims:
      self.info[name].ylims = ylims

    if selections:
      self.info[name].selections = selections

  def setup_resources(self, colors:list=None, fits:list=None, ax:Axes=None, final_call=(lambda x : x.ax.set_ylabel("% Resource")), selections:list=["DSP %", "FF %", "LUT %"], ylims=None):
    self._setup("resources", colors, fits, ax, final_call, selections, ylims)
    return self

  def setup_latency(self, colors:list=None, fits:list=None, ax:Axes=None, final_call=(lambda x : x.ax.set_ylabel("Latency (cycles)")), selections:list=["Latency (cycles)"], ylims=None):
    self._setup("latency", colors, fits, ax, final_call, selections, ylims)
    return self

  def setup_compile(self, colors:list=None, fits:list=None, ax:Axes=None, final_call=(lambda x : x.ax.set_ylabel("Compile Time (mins)")), selections:list=["Timing (min)"], ylims=None):
    self._setup("compile", colors, fits, ax, final_call, selections, ylims)
    return self

  def setup(self, name, colors:list=None, fits:list=None, ax:Axes=None, final_call=None, selections:list=None, ylims=None):
    '''
    fit_funcs choices: ["linear", "quad", "exp"]
    '''
    assert name is not None
    # assert selections is not None # allow re-updating the same entry
    self._setup(name, colors, fits, ax, final_call, selections, ylims)
    return self
  
  def defaults(self, axRes, axLat, axComp, axDSP, axFF, axLUT, colors=["red", "green", "blue"]):
    resYLabel = (lambda x : x.ax.set_ylabel("% Resource"))

    self.setup_resources(colors=colors, fits=["linear", "linear", "linear"], ax=axRes)
    self.setup_latency(fits=["quad"], ax=axLat)
    self.setup_compile(fits=["quad"], ax=axComp)
    self.setup(name="DSP", fits=["linear"], final_call=resYLabel, ax=axDSP, selections=["DSP %"])
    self.setup(name="FF", fits=["linear"], final_call=resYLabel, ax=axFF, selections=["FF %"])
    self.setup(name="LUT", fits=["linear"], final_call=resYLabel, ax=axLUT, selections=["LUT %"])

    return self

  def plot(self, xlims: list = [0,None]):
    for key, info in self.info.items():
      assert isinstance(info, PlotInfo) # for intellisense?
      assert len(info.selections) == len(info.colors) and len(info.colors) == len(info.fits)

      # if info.ylims is not None:
      #   info.ax.set_ylim(info.ylims)
        
      for s, c, ff in zip(info.selections, info.colors, info.fits):
        assert isinstance(ff, FitFunc)
        track_sizes = np.array(self.track_sizes)
        if xlims[1] is None:
          d = self.data[s]
        else: # limit x for plotting and data
          track_sizes = track_sizes[track_sizes <= xlims[1]]
          d = self.data[self.data["Track Size"] <= xlims[1]][s]

        info.ax.scatter(track_sizes, d, label=str(self.ID) + " " + s, edgecolor='black', color=c, zorder=3, s=20)
        lastx = track_sizes[-1]
        lasty = d.to_numpy()[-1]
        if xlims[1] is None:
          xlims[1] = lastx
        if (len(track_sizes) > 1 and ff.name == 'linear') or len(track_sizes) > 2:
          f = ff.f
          pars, cov = curve_fit(f, track_sizes, d, ff.p0)
          x = np.linspace(xlims[0], xlims[1], 100)
          # print(pars)
          y = f(x, *pars)
          yp = f(track_sizes, *pars)
          SSres = np.sum(np.power(yp - d.to_numpy(),2))
          SStot = np.sum(np.power(yp - d.mean(), 2))
          if abs(SStot) < 0.0001: # so that it doesnt divide by a small number and break
            r2 = 1
          else:
            r2 = 1 - SSres/SStot
          # label = "{:.1f} " * len(pars)
          # label = label.format(*pars)
          pars = np.append(pars, r2)
          label = ff.to_string(*pars)
          print(self.ID, s, label)
          label = None # TODO: tmp
          info.ax.plot(x, y, label=label, zorder=2, linewidth=0.5, color=c, linestyle='--')
          info.ax.text(lastx, lasty, "  {} - {}".format(self.ID, ff.name))
        else:
          info.ax.text(lastx, lasty, "  {}".format(self.ID))



      info.ax.grid(True, zorder=1)
      info.ax.scatter([0],[0], color='white', s=1)
      info.ax.legend(loc="upper left") # TODO: tmp
      info.ax.set_xlabel("Track Size (count)")
      # info.ax.set_title("Module {}".format(self.modules)) # TODO: tmp
      info.ax.set_title("{}".format(", ".join(info.selections)))
      info.final_call(info)
