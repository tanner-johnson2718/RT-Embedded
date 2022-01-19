

export CHEDDAR_INSTALL_PATH=/home/tj/Desktop/RT-Embedded/week3/Cheddar-3.2-Linux64-bin/

export PATH=$CHEDDAR_INSTALL_PATH:$PATH

export LIBRARY_PATH=$CHEDDAR_INSTALL_PATH/lib/linux64/gtk-2.0:\
$CHEDDAR_INSTALL_PATH/lib/linux64:\
/usr/lib64:\
/usr/lib/x86_64-linux-gnu:\
/lib/x86_64-linux-gnu:\
$LIBRARY_PATH

export LD_LIBRARY_PATH=:$LIBRARY_PATH:$LD_LIBRARY_PATH








