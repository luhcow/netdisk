document.addEventListener('DOMContentLoaded', () => {
    fetchFileList('');  // 初始化加载根目录
});

let currentPath = '';  // 记录当前的路径

// 更新文件列表并动态刷新页面
function fetchFileList(path) {
    currentPath = path;  // 更新当前路径
    fetch('/files', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json',
        },
        body: JSON.stringify({ path })  // 将路径作为POST请求的body
    })
        .then(response => response.json())
        .then(data => {
            const fileList = document.getElementById('fileList').getElementsByTagName('tbody')[0];
            const breadcrumb = document.getElementById('breadcrumb');

            fileList.innerHTML = '';  // 清空文件列表
            breadcrumb.innerHTML = '';  // 清空路径导航

            // 动态生成路径导航
            const pathParts = path.split('/').filter(Boolean);
            let accumulatedPath = '';
            breadcrumb.innerHTML = `<a href="#" onclick="navigateTo('')">根目录</a> / `;
            pathParts.forEach((part, index) => {
                accumulatedPath += (index > 0 ? '/' : '') + part;
                breadcrumb.innerHTML += `<a href="#" onclick="navigateTo('${accumulatedPath}')">${part}</a> / `;
            });

            // 动态生成文件列表
            data.forEach(item => {
                const row = fileList.insertRow();
                row.innerHTML = `
                <td><a href="#" onclick="navigateTo('${path}/${item.name}')">${item.name}</a></td>
                <td>${item.isDirectory ? '文件夹' : '文件'}</td>
                <td>${item.isDirectory ? '-' : (item.size / 1024).toFixed(2) + ' KB'}</td>
                <td>
                    ${item.isDirectory ? '' : `<button onclick="downloadFile('${path}/${item.name}')">下载</button>`}
                    ${!item.isDirectory ? `<button onclick="deleteFile('${path}/${item.name}')">删除</button>` : ''}
                </td>
            `;
            });
        })
        .catch(error => console.error('Error fetching file list:', error));
}

// 上传文件
function uploadFile() {
    const fileInput = document.getElementById('fileInput');
    const file = fileInput.files[0];

    if (file) {
        const formData = new FormData();
        formData.append('file', file);
        formData.append('path', currentPath);  // 传递当前路径

        fetch('/upload', {
            method: 'POST',
            body: formData
        })
            .then(response => response.json())
            .then(result => {
                alert(result.message);
                fetchFileList(currentPath);  // 上传成功后刷新文件列表
            })
            .catch(error => console.error('Error uploading file:', error));
    } else {
        alert('请选择一个文件');
    }
}

// 下载文件
function downloadFile(filePath) {
    fetch('/download', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json',
        },
        body: JSON.stringify({ path: filePath })  // 将文件路径作为POST请求的body
    })
        .then(response => {
            if (response.ok) {
                return response.blob();
            } else {
                throw new Error('下载失败');
            }
        })
        .then(blob => {
            const url = window.URL.createObjectURL(blob);
            const a = document.createElement('a');
            a.href = url;
            a.download = filePath.split('/').pop();  // 从路径中提取文件名
            document.body.appendChild(a);
            a.click();
            a.remove();
        })
        .catch(error => console.error('Error downloading file:', error));
}

// 删除文件
function deleteFile(filePath) {
    fetch('/delete', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json',
        },
        body: JSON.stringify({ path: filePath })  // 将文件路径作为POST请求的body
    })
        .then(response => response.json())
        .then(result => {
            alert(result.message);
            fetchFileList(currentPath);  // 删除成功后刷新文件列表
        })
        .catch(error => console.error('Error deleting file:', error));
}

// 导航到指定路径
function navigateTo(path) {
    fetchFileList(path);
}
