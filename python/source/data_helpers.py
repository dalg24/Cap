__all__=[
    'initialize_data',
    'report_data',
    'plot_data',
]

from matplotlib import pyplot
from numpy import array,append

def initialize_data():
    return {
        'time'   :array([],dtype=float),
        'current':array([],dtype=float),
        'voltage':array([],dtype=float),
    }

def report_data(data,time,device):
    data['time'   ]=append(data['time'   ],time                )
    data['current']=append(data['current'],device.get_current())
    data['voltage']=append(data['voltage'],device.get_voltage())

def plot_data(data):
    time   =data['time'   ]
    current=data['current']
    voltage=data['voltage']
    plot_linewidth=3
    label_fontsize=20
    f,axarr=pyplot.subplots(2,sharex=True,figsize=(16,12))
    axarr[0].plot(time,1e3*current,lw=plot_linewidth)
    axarr[0].set_ylabel(r'$\mathrm{Current\ [mA]}$',fontsize=label_fontsize)
    axarr[1].plot(time,voltage,lw=plot_linewidth)
    axarr[1].set_ylabel(r'$\mathrm{Voltage\  [V]}$',fontsize=label_fontsize)
    axarr[1].set_xlabel(r'$\mathrm{Time\     [s]}$',fontsize=label_fontsize)
    pyplot.show()
