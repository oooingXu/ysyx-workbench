nommulinux_minirv_gdb:
	@gdb -s /home/romeo/ysyx-workbench/nemu/build/riscv32-nemu-interpreter --args /home/romeo/ysyx-workbench/nemu/build/riscv32-nemu-interpreter --log=/home/romeo/ysyx-workbench/nemu/build/nemu-log.txt --diff=/home/romeo/ysyx-workbench/nemu/tools/spike-diff/build/riscv32-spike-so /home/romeo/ysyx-workbench/nemu/resource/nommulinux/DownloadedImage
	
nommulinux_minirv_sdb:
	@/home/romeo/ysyx-workbench/nemu/build/riscv32-nemu-interpreter --log=/home/romeo/ysyx-workbench/nemu/build/nemu-log.txt --diff=/home/romeo/mini-rv32ima/mini-rv32ima/build/riscv32-mini-rv32ima-so /home/romeo/ysyx-workbench/nemu/resource/nommulinux/DownloadedImage

nommulinux_minirv:
	@/home/romeo/ysyx-workbench/nemu/build/riscv32-nemu-interpreter --log=/home/romeo/ysyx-workbench/nemu/build/nemu-log.txt --diff=/home/romeo/mini-rv32ima/mini-rv32ima/build/riscv32-mini-rv32ima-so -b /home/romeo/ysyx-workbench/nemu/resource/nommulinux/DownloadedImage

nommulinux:
	@/home/romeo/ysyx-workbench/nemu/build/riscv32-nemu-interpreter --log=/home/romeo/ysyx-workbench/nemu/build/nemu-log.txt --diff=/home/romeo/ysyx-workbench/nemu/tools/spike-diff/build/riscv32-spike-so -b /home/romeo/ysyx-workbench/nemu/resource/nommulinux/Image

nommulinux_sdb:
	@/home/romeo/ysyx-workbench/nemu/build/riscv32-nemu-interpreter --log=/home/romeo/ysyx-workbench/nemu/build/nemu-log.txt --diff=/home/romeo/ysyx-workbench/nemu/tools/spike-diff/build/riscv32-spike-so /home/romeo/ysyx-workbench/nemu/resource/nommulinux/Image

nommulinux_gdb:
	@gdb -s /home/romeo/ysyx-workbench/nemu/build/riscv32-nemu-interpreter --args /home/romeo/ysyx-workbench/nemu/build/riscv32-nemu-interpreter --log=/home/romeo/ysyx-workbench/nemu/build/nemu-log.txt --diff=/home/romeo/ysyx-workbench/nemu/tools/spike-diff/build/riscv32-spike-so /home/romeo/ysyx-workbench/nemu/resource/nommulinux/Image
	
nommulinux_diff_minirv_sdb:
	@/home/romeo/ysyx-workbench/nemu/build/riscv32-nemu-interpreter --log=/home/romeo/ysyx-workbench/nemu/build/nemu-log.txt --diff=/home/romeo/mini-rv32ima/mini-rv32ima/build/riscv32-mini-rv32ima-so /home/romeo/ysyx-workbench/nemu/resource/nommulinux/Image

nommulinux_diff_minirv:
	@/home/romeo/ysyx-workbench/nemu/build/riscv32-nemu-interpreter --log=/home/romeo/ysyx-workbench/nemu/build/nemu-log.txt --diff=/home/romeo/mini-rv32ima/mini-rv32ima/build/riscv32-mini-rv32ima-so -b /home/romeo/ysyx-workbench/nemu/resource/nommulinux/Image

