# 简单爬虫实现（仅供学习，请勿恶意攻击）

```python
import requests
from bs4 import BeautifulSoup
import os
from urllib.parse import urljoin
import time
import random

# 配置参数
base_url = "https://mirrors.tuna.tsinghua.edu.cn/kernel/"
save_root = "./kernel_downloads"  # 本地保存根目录
headers = {
    'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:136.0) Gecko/20100101 Firefox/136.0'
}
delay = 1  # 请求间隔时间（秒）

def download_file(url, file_path):
    """下载文件到本地"""
    if os.path.exists(file_path):
        print(f"文件已存在，跳过: {file_path}")
        return
    
    try:
        with requests.get(url, headers=headers, stream=True, timeout=30) as r:
            r.raise_for_status()
            with open(file_path, 'wb') as f:
                for chunk in r.iter_content(chunk_size=8192):
                    f.write(chunk)
        print(f"下载成功: {file_path}")
    except Exception as e:
        print(f"下载失败 {url}: {str(e)}")

def process_directory(url, local_path):
    """处理目录页面"""
    print(f"正在处理目录: {url}")
    
    # 创建本地目录
    os.makedirs(local_path, exist_ok=True)
    
    try:
        response = requests.get(url, headers=headers, timeout=30)
        response.raise_for_status()
    except Exception as e:
        print(f"访问失败 {url}: {str(e)}")
        return
    
    soup = BeautifulSoup(response.text, 'html.parser')
    
    # 解析所有链接（排除父目录链接）
    for link in soup.select('a[href]:not([href="../"])'):
        href = link['href']
        absolute_url = urljoin(url, href)
        print(absolute_url)
        
        # 过滤非目标URL
        if not absolute_url.startswith(base_url):
            print(f"error 1:{absolute_url}")
            continue
        
        # 处理子目录
        if href.endswith('/'):
            child_dir = os.path.join(local_path, href.rstrip('/'))
            process_directory(absolute_url, child_dir)
        
        # 处理文件
        else:
            file_name = href.split('?')[0]  # 去除URL参数
            file_path = os.path.join(local_path, file_name)
            download_file(absolute_url, file_path)
        
        time.sleep(delay + random.randint(0,10)/10.0)  # 延迟

if __name__ == "__main__":
    process_directory(base_url, save_root)
    print("任务执行完毕")

```
