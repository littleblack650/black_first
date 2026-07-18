#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
W25Q64烧录工具 - GUI版本
"""

import tkinter as tk
from tkinter import ttk, filedialog, messagebox, scrolledtext
import threading
import queue
import serial.tools.list_ports
from w25q64_burn_tool import W25Q64Burner, ImageParser, BatchBurner, ImageInfo


class BurnToolGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("W25Q64 Flash烧录工具")
        self.root.geometry("800x600")

        # 状态变量
        self.is_connected = False
        self.burner = None
        self.burn_thread = None
        self.stop_burn = False
        self.log_queue = queue.Queue()

        # 创建UI
        self.create_widgets()

        # 开始日志处理线程
        self.root.after(100, self.process_log_queue)

    def create_widgets(self):
        # 顶部框架
        top_frame = ttk.Frame(self.root, padding="10")
        top_frame.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))

        # 串口设置
        ttk.Label(top_frame, text="串口:").grid(row=0, column=0, sticky=tk.W)

        self.port_var = tk.StringVar()
        self.port_combo = ttk.Combobox(top_frame, textvariable=self.port_var, width=15)
        self.port_combo.grid(row=0, column=1, padx=5, sticky=tk.W)
        self.refresh_ports()

        ttk.Button(top_frame, text="刷新", command=self.refresh_ports).grid(row=0, column=2, padx=5)

        ttk.Label(top_frame, text="波特率:").grid(row=0, column=3, padx=(20, 5), sticky=tk.W)
        self.baudrate_var = tk.StringVar(value="115200")
        self.baudrate_combo = ttk.Combobox(top_frame, textvariable=self.baudrate_var,
                                           values=["9600", "19200", "38400", "57600", "115200", "230400"],
                                           width=10)
        self.baudrate_combo.grid(row=0, column=4, padx=5, sticky=tk.W)

        self.connect_btn = ttk.Button(top_frame, text="连接", command=self.toggle_connection)
        self.connect_btn.grid(row=0, column=5, padx=20)

        # 文件选择框架
        file_frame = ttk.LabelFrame(self.root, text="文件设置", padding="10")
        file_frame.grid(row=1, column=0, padx=10, pady=10, sticky=(tk.W, tk.E, tk.N, tk.S))

        # 地址文件
        ttk.Label(file_frame, text="地址文件:").grid(row=0, column=0, sticky=tk.W)
        self.address_file_var = tk.StringVar()
        ttk.Entry(file_frame, textvariable=self.address_file_var, width=40).grid(row=0, column=1, padx=5)
        ttk.Button(file_frame, text="浏览", command=self.browse_address_file).grid(row=0, column=2)

        # 图片文件
        ttk.Label(file_frame, text="图片文件:").grid(row=1, column=0, sticky=tk.W, pady=5)
        self.picture_file_var = tk.StringVar()
        ttk.Entry(file_frame, textvariable=self.picture_file_var, width=40).grid(row=1, column=1, padx=5, pady=5)
        ttk.Button(file_frame, text="浏览", command=self.browse_picture_file).grid(row=1, column=2, pady=5)

        # 解析按钮
        ttk.Button(file_frame, text="解析文件", command=self.parse_files).grid(row=2, column=1, pady=10)

        # 烧录设置框架
        settings_frame = ttk.LabelFrame(self.root, text="烧录设置", padding="10")
        settings_frame.grid(row=2, column=0, padx=10, pady=10, sticky=(tk.W, tk.E, tk.N, tk.S))

        self.verify_var = tk.BooleanVar(value=True)
        ttk.Checkbutton(settings_frame, text="烧录后验证", variable=self.verify_var).grid(row=0, column=0, sticky=tk.W)

        self.skip_erase_var = tk.BooleanVar(value=False)
        ttk.Checkbutton(settings_frame, text="跳过擦除", variable=self.skip_erase_var).grid(row=0, column=1, padx=20,
                                                                                            sticky=tk.W)

        # 烧录控制框架
        control_frame = ttk.Frame(self.root, padding="10")
        control_frame.grid(row=3, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))

        self.burn_btn = ttk.Button(control_frame, text="开始烧录", command=self.start_burn, state=tk.DISABLED)
        self.burn_btn.grid(row=0, column=0, padx=5)

        self.stop_btn = ttk.Button(control_frame, text="停止", command=self.stop_burn_process, state=tk.DISABLED)
        self.stop_btn.grid(row=0, column=1, padx=5)

        # 进度条
        self.progress_var = tk.DoubleVar()
        self.progress_bar = ttk.Progressbar(control_frame, variable=self.progress_var, maximum=100)
        self.progress_bar.grid(row=0, column=2, padx=20, sticky=(tk.W, tk.E))

        # 日志框架
        log_frame = ttk.LabelFrame(self.root, text="日志", padding="10")
        log_frame.grid(row=4, column=0, padx=10, pady=10, sticky=(tk.W, tk.E, tk.N, tk.S))

        # 配置网格权重
        self.root.columnconfigure(0, weight=1)
        self.root.rowconfigure(4, weight=1)
        log_frame.columnconfigure(0, weight=1)
        log_frame.rowconfigure(0, weight=1)

        # 日志文本框
        self.log_text = scrolledtext.ScrolledText(log_frame, height=15, wrap=tk.WORD)
        self.log_text.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))

        # 状态栏
        self.status_var = tk.StringVar(value="就绪")
        status_bar = ttk.Label(self.root, textvariable=self.status_var, relief=tk.SUNKEN)
        status_bar.grid(row=5, column=0, sticky=(tk.W, tk.E), padx=10, pady=5)

    def refresh_ports(self):
        """刷新串口列表"""
        ports = serial.tools.list_ports.comports()
        port_list = [port.device for port in ports]
        self.port_combo['values'] = port_list
        if port_list:
            self.port_var.set(port_list[0])

    def browse_address_file(self):
        """浏览地址文件"""
        filename = filedialog.askopenfilename(
            title="选择地址文件",
            filetypes=[("文本文件", "*.txt"), ("所有文件", "*.*")]
        )
        if filename:
            self.address_file_var.set(filename)

    def browse_picture_file(self):
        """浏览图片文件"""
        filename = filedialog.askopenfilename(
            title="选择图片文件",
            filetypes=[("文本文件", "*.txt"), ("所有文件", "*.*")]
        )
        if filename:
            self.picture_file_var.set(filename)

    def log(self, message):
        """添加日志"""
        self.log_queue.put(message)

    def process_log_queue(self):
        """处理日志队列"""
        try:
            while True:
                message = self.log_queue.get_nowait()
                self.log_text.insert(tk.END, message + "\n")
                self.log_text.see(tk.END)
                self.log_text.update()
        except queue.Empty:
            pass

        self.root.after(100, self.process_log_queue)

    def toggle_connection(self):
        """连接/断开设备"""
        if not self.is_connected:
            # 连接设备
            port = self.port_var.get()
            baudrate = int(self.baudrate_var.get())

            if not port:
                messagebox.showerror("错误", "请选择串口")
                return

            self.burner = W25Q64Burner(port, baudrate)

            if self.burner.connect():
                self.is_connected = True
                self.connect_btn.config(text="断开")
                self.status_var.set(f"已连接到 {port}")
                self.log(f"已连接到 {port}，波特率 {baudrate}")

                # 获取Flash信息
                try:
                    flash_info = self.burner.get_flash_info()
                    if flash_info:
                        self.log(f"Flash容量: {flash_info.capacity / 1024 / 1024:.1f} MB")
                        self.log(f"扇区大小: {flash_info.sector_size} 字节")
                        self.log(f"页大小: {flash_info.page_size} 字节")
                except Exception as e:
                    self.log(f"获取Flash信息失败: {e}")

                # 启用烧录按钮
                self.burn_btn.config(state=tk.NORMAL)
            else:
                self.log("连接失败")
        else:
            # 断开设备
            if self.burner:
                self.burner.disconnect()
                self.burner = None

            self.is_connected = False
            self.connect_btn.config(text="连接")
            self.status_var.set("已断开")
            self.log("已断开连接")
            self.burn_btn.config(state=tk.DISABLED)

    def parse_files(self):
        """解析文件"""
        address_file = self.address_file_var.get()
        picture_file = self.picture_file_var.get()

        if not address_file or not picture_file:
            messagebox.showerror("错误", "请选择地址文件和图片文件")
            return

        try:
            self.log("解析文件中...")

            # 解析地址文件
            addresses = ImageParser.parse_address_file(address_file)
            self.log(f"找到 {len(addresses)} 个地址")

            # 解析图片文件
            images_raw = ImageParser.parse_picture_file(picture_file)
            self.log(f"找到 {len(images_raw)} 张图片")

            if len(addresses) != len(images_raw):
                self.log(f"警告: 地址数量({len(addresses)})与图片数量({len(images_raw)})不匹配")

            self.image_infos = []
            for i, (address, (name, data)) in enumerate(zip(addresses, images_raw)):
                if i >= len(addresses):
                    break

                # 验证图片大小
                if not ImageParser.validate_image_size(data):
                    self.log(f"警告: 图片 {name} 大小异常")

                image_info = ImageInfo(
                    address=address,
                    data=data,
                    name=name
                )
                self.image_infos.append(image_info)

            self.log(f"成功解析 {len(self.image_infos)} 张图片")
            self.status_var.set(f"就绪 - {len(self.image_infos)} 张图片待烧录")

        except Exception as e:
            self.log(f"解析文件失败: {e}")
            messagebox.showerror("错误", f"解析文件失败:\n{e}")

    def start_burn(self):
        """开始烧录"""
        if not hasattr(self, 'image_infos') or not self.image_infos:
            messagebox.showerror("错误", "请先解析文件")
            return

        # 禁用按钮，启用停止按钮
        self.burn_btn.config(state=tk.DISABLED)
        self.stop_btn.config(state=tk.NORMAL)
        self.connect_btn.config(state=tk.DISABLED)

        # 重置进度
        self.progress_var.set(0)

        # 启动烧录线程
        self.stop_burn = False
        self.burn_thread = threading.Thread(target=self.burn_process)
        self.burn_thread.start()

    def burn_process(self):
        """烧录进程"""
        try:
            verify = self.verify_var.get()
            skip_erase = self.skip_erase_var.get()

            batch_burner = BatchBurner(self.burner)

            if not skip_erase:
                self.log("擦除Flash...")

                # 获取扇区大小
                flash_info = self.burner.get_flash_info()
                sector_size = flash_info.sector_size if flash_info else 4096

                # 获取需要擦除的唯一扇区
                unique_sectors = set(img.address & ~(sector_size - 1) for img in self.image_infos)

                for i, sector in enumerate(unique_sectors):
                    if self.stop_burn:
                        self.log("烧录被用户中断")
                        break

                    self.log(f"擦除扇区: 0x{sector:06X}")
                    if not self.burner.erase_sector(sector):
                        self.log(f"错误: 擦除扇区失败: 0x{sector:06X}")
                        return

                    # 更新进度
                    progress = (i + 1) / len(unique_sectors) * 30  # 擦除占30%
                    self.progress_var.set(progress)

            if self.stop_burn:
                return

            # 烧录图片
            self.log("开始烧录图片...")
            total_images = len(self.image_infos)

            for i, image in enumerate(self.image_infos):
                if self.stop_burn:
                    self.log("烧录被用户中断")
                    break

                self.log(f"[{i + 1}/{total_images}] {image.name} - 地址: 0x{image.address:06X}")

                # 写入图片
                if not self.burner.write_page(image.address, image.data):
                    self.log(f"错误: 写入图片失败: {image.name}")
                    return

                # 验证图片
                if verify:
                    if not self.burner.verify_data(image.address, image.data):
                        self.log(f"错误: 验证图片失败: {image.name}")
                        return
                    self.log("  验证通过")

                # 更新进度
                progress = 30 + (i + 1) / total_images * 70  # 烧录占70%
                self.progress_var.set(progress)

            if not self.stop_burn:
                self.log("烧录完成!")
                self.status_var.set("烧录完成")
                self.progress_var.set(100)

        except Exception as e:
            self.log(f"烧录失败: {e}")
            import traceback
            self.log(traceback.format_exc())

        finally:
            # 恢复按钮状态
            self.root.after(0, self.on_burn_complete)

    def on_burn_complete(self):
        """烧录完成回调"""
        self.burn_btn.config(state=tk.NORMAL)
        self.stop_btn.config(state=tk.DISABLED)
        self.connect_btn.config(state=tk.NORMAL)

    def stop_burn_process(self):
        """停止烧录"""
        self.stop_burn = True
        self.stop_btn.config(state=tk.DISABLED)
        self.log("正在停止烧录...")


def main():
    root = tk.Tk()
    app = BurnToolGUI(root)
    root.mainloop()


if __name__ == "__main__":
    main()