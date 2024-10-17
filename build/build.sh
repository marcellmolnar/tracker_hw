export PICO_SDK_PATH=/home/m/rpi/pico-sdk/
export PICO_EXTRAS_PATH=/home/m/rpi/pico-extras
cmake .. -DPICO_BOARD=waveshare_rp2040_zero
make
cp blink.uf2 /media/shares/users/marcell.molnar/rpi/
