const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="zh-CN">
<meta charset="UTF-8">
<head>
    <title>TF卡文件在线管理系统</title>
    <meta name='viewport' content='user-scalable=yes,initial-scale=1.0,width=device-width'>
    <style>
        body {
            max-width: 65%;
            margin: auto;
            font-family: system-ui;
        }

        ul {
            list-style-type: none;
            padding: 0;
            border-radius: 0.5em;
            overflow: hidden;
            background-color: #66ccff;
        }

        li {
            float: left;
        }

        li a {
            display: block;
            padding: 0.44em 0.44em;
        }

        h1 {
            padding: 0.2em 0.2em;
            background: #e14343;
            text-align: center;
        }

        table {
            font-family: sans-serif;
            font-size: 0.9em;
            border-collapse: collapse;
            width: 95%;
        }

        th, td {
            text-align: center;
            padding: 0.3em;
            border: 0.06em solid #dddddd;
        }

        tr:nth-child(odd) {
            background-color: #eeeeee;
        }

    </style>
</head>
<body>
<h1 id="status">TF卡状态:未连接</h1>
<ul>
    <li><a onclick='listFiles()'>文件列表</a></li>
    <li><a onclick='addUploadForm()'>上传文件</a></li>
    <li><a onclick='connectTF()'>连接TF卡</a></li>
    <li><a onclick='disconnectTF()'>断开(TF卡连接外部设备)</a></li>
</ul>

<script>
    let xhr = new XMLHttpRequest();
    xhr.open("GET", "/state");
    xhr.onload = function () {
        if (xhr.status === 200) {
            let response = xhr.responseText;
            if (response === "1") {
                updateStatus(true);
            } else {
                updateStatus(false);
            }
        }
    };
    xhr.send();

    function updateStatus(isConnected) {
        let statusElement = document.getElementById('status');
        if (isConnected) {
            statusElement.textContent = 'TF卡状态:已连接';
            statusElement.style.background = '#30cc28';
        } else {
            statusElement.textContent = 'TF卡状态:未连接';
            statusElement.style.background = '#e14343';
        }
    }

    let elements = {};

    function clearElements() {
        for (let key in elements) {
            if (elements[key] && elements[key].parentNode) {
                elements[key].parentNode.removeChild(elements[key]);
            }
            delete elements[key];
        }
    }

    function listFiles() {
        clearElements();
        // 创建表格
        let table = document.createElement('table');
        table.id = 'fileTable';
        elements['table'] = table;
        // 创建表头
        let headerRow = table.insertRow(-1);
        let headerCell1 = document.createElement('th');
        headerCell1.innerHTML = '文件名';
        headerRow.appendChild(headerCell1);
        let headerCell2 = document.createElement('th');
        headerCell2.innerHTML = '大小';
        headerRow.appendChild(headerCell2);
        let headerCell3 = document.createElement('th');
        headerCell3.innerHTML = '下载';
        headerRow.appendChild(headerCell3);
        let headerCell4 = document.createElement('th');
        headerCell4.innerHTML = '删除';
        headerRow.appendChild(headerCell4);

        document.body.appendChild(table);

        let xhr = new XMLHttpRequest();
        xhr.open("GET", "/getFiles");
        xhr.onreadystatechange = function () {
            if (xhr.readyState === 4 && xhr.status === 200) {
                let response = JSON.parse(xhr.responseText);
                for (let i = 0; i < response.length; i++) {
                    let row = table.insertRow(-1);
                    let cell1 = row.insertCell(0);
                    let cell2 = row.insertCell(1);
                    let cell3 = row.insertCell(2);
                    let cell4 = row.insertCell(3);

                    cell1.innerHTML = response[i].name;
                    cell2.innerHTML = response[i].size;

                    let deleteBtn = document.createElement('button');
                    deleteBtn.innerHTML = '删除';
                    deleteBtn.onclick = function () {
                        let xhr = new XMLHttpRequest();
                        xhr.open("GET", "/delete?file=" + encodeURIComponent(response[i].name));
                        xhr.onload = function () {
                            if (xhr.status === 200) {
                                listFiles();
                            }
                        };
                        xhr.send();
                    };

                    let downloadBtn = document.createElement('button');
                    downloadBtn.innerHTML = '下载';
                    downloadBtn.onclick = function () {
                        let link = document.createElement('a');
                        link.href = "/download?file=" + encodeURIComponent(response[i].name);
                        link.download = response[i].name;
                        link.click();
                    };
                    cell3.appendChild(downloadBtn);
                    cell4.appendChild(deleteBtn);
                }
            }
        }
        xhr.send();
    }

    function disconnectTF() {
        clearElements();
        const hint = document.createElement('h3');
        elements['hint'] = hint;
        hint.innerText = '断开中';
        document.body.appendChild(hint);
        let xhr = new XMLHttpRequest();
        xhr.open("GET", '/off');
        xhr.onload = function () {
            if (xhr.status === 200) {
                hint.innerText = '断开成功';
                updateStatus(false);
            }
        }
        xhr.send();
    }

    function connectTF() {
        clearElements();
        const hint = document.createElement('h3');
        elements['hint'] = hint;
        hint.innerText = '连接中';
        document.body.appendChild(hint);
        let xhr = new XMLHttpRequest();
        xhr.open("GET", '/on');
        xhr.onload = function () {
            if (xhr.status === 200) {
                let response = xhr.responseText;
                if (response === '1') {
                    hint.innerText = '连接成功';
                    updateStatus(true);
                } else {
                    hint.innerText = '连接失败';
                    updateStatus(false);
                }
            }
        }
        xhr.send();
    }

    function addUploadForm() {
        clearElements();
        const form = document.createElement('form');
        form.enctype = 'multipart/form-data';
        elements['uploadForm'] = form;

        const fileInput = document.createElement('input');
        fileInput.type = 'file';
        elements['fileInput'] = fileInput;

        const button = document.createElement('button');
        button.type = 'button';
        button.innerText = '上传';
        button.addEventListener('click', uploadFile);
        elements['button'] = button;

        const hint = document.createElement('h3');
        hint.innerText = '点击上传按钮后请耐心等待,不要离开界面!';
        elements['hint'] = hint;

        const progressBar = document.createElement('progress');
        progressBar.id = 'progressBar';
        progressBar.value = 0;
        progressBar.max = 100;
        elements['progressBar'] = progressBar;

        const text = document.createElement('h4');
        text.id = 'uploadSize';
        elements['uploadSize'] = text;

        form.appendChild(fileInput);
        form.appendChild(button);
        form.appendChild(hint);
        form.appendChild(progressBar);
        form.appendChild(text);
        document.body.appendChild(form);
    }

    function uploadFile() {
        const file = elements['fileInput'].files[0];
        let formData = new FormData();
        formData.append("file", file);
        let xhr = new XMLHttpRequest();
        xhr.upload.addEventListener("progress", progressHandler, false);
        xhr.addEventListener("load", completeHandler, false);
        xhr.addEventListener("error", errorHandler, false);
        xhr.addEventListener("abort", abortHandler, false);
        xhr.open("POST", "/fileUpload");
        xhr.send(formData);
    }

    function progressHandler(event) {
        elements['uploadSize'].innerText = formatBytes(event.loaded)
        let percent = (event.loaded / event.total) * 100;
        elements['progressBar'].value = Math.round(percent);
    }

    function completeHandler(event) {
        elements['uploadSize'].innerText = '上传完成'
    }

    function errorHandler(event) {
        elements['uploadSize'].innerText = '上传失败'
    }

    function abortHandler(event) {
        elements['uploadSize'].innerText = '上传中止'
    }

    function formatBytes(bytes, decimals = 2) {
        if (bytes === 0) return '0 B';
        const k = 1024;
        const dm = decimals < 0 ? 0 : decimals;
        const sizes = ['B', 'KB', 'MB', 'GB'];
        const i = Math.floor(Math.log(bytes) / Math.log(k));
        return parseFloat((bytes / Math.pow(k, i)).toFixed(dm)) + ' ' + sizes[i];
    }
</script>
</body>
</html>
)rawliteral";