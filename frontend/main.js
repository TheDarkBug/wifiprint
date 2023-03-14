const TOKEN = "";
const CHAT_ID = "";
const TG_URL = `https://api.telegram.org/bot${TOKEN}`;
let upload_count = 0;
let page_count = 0;
let completed_ups = [];

function sendDoc(fname, ftype, content) {
  const xhr = new XMLHttpRequest();
  const progressSpan = document.createElement("span");
  progressSpan.className = "bar";
  const local_upid = upload_count++;
  completed_ups.push(false);
  const progressDiv = document.getElementById("upload-progress");
  const localDiv = document.createElement("div");
  localDiv.className = "upload-container";
  localDiv.appendChild(progressSpan);
  const label = document.createElement("span");
  label.textContent = fname;
  label.className = "label";
  progressDiv.appendChild(localDiv);
  progressDiv.appendChild(label);
  progressDiv.appendChild(document.createElement("br"));
  xhr.open("POST", TG_URL + "/sendDocument", true);
  xhr.upload.onprogress = function (event) {
    const percentComplete = (event.loaded / event.total) * 100;
    progressSpan.style.width = `${percentComplete}%`;
    if (percentComplete === 100) {
      progressSpan.className += " completed";
      completed_ups[local_upid] = true;
    }
  };
  const file = new Blob([content], { type: ftype });
  const formData = new FormData();
  formData.append("chat_id", CHAT_ID);
  formData.append("document", file, fname);

  xhr.send(formData);
}

function getFileType(content) {
  const dataView = new DataView(content);
  const firstBytes = new Uint8Array(dataView.buffer.slice(0, 4));
  let fileType = "";

  if (firstBytes[0] === 0x89 && firstBytes[1] === 0x50 && firstBytes[2] === 0x4e && firstBytes[3] === 0x47) {
    fileType = "image/png";
  } else if (firstBytes[0] === 0xff && firstBytes[1] === 0xd8 && firstBytes[2] === 0xff) {
    fileType = "image/jpeg";
  } else if (firstBytes[0] === 0x47 && firstBytes[1] === 0x49 && firstBytes[2] === 0x46) {
    fileType = "image/gif";
  } else if (firstBytes[0] === 0x42 && firstBytes[1] === 0x4d) {
    fileType = "image/bmp";
  } else if (firstBytes[0] === 0x25 && firstBytes[1] === 0x50 && firstBytes[2] === 0x44 && firstBytes[3] === 0x46) {
    fileType = "application/pdf";
  } else {
    fileType = "application/octet-stream";
  }
  return fileType;
}

function openFileDialog() {
  var input = document.createElement("input");
  input.type = "file";
  input.multiple = true;
  input.addEventListener("change", function () {
    for (const file of this.files) {
      const reader = new FileReader();
      if (file.size > 1024 * 1024 * 50 - 304) {
        alert("File is too big!");
        return;
      }
      reader.readAsArrayBuffer(file);
      reader.addEventListener("load", function () {
        const fileType = getFileType(reader.result);
        if (fileType === "application/pdf") page_count += getPageCount(reader.result);
        else page_count++;
        let checkArrayValues = setInterval(function () {
          if (completed_ups.length > 0)
            if (completed_ups.every((element) => element === true)) {
              fetch(
                `${TG_URL}/sendMessage?chat_id=${CHAT_ID}&text=${encodeURIComponent(
                  `Pagine: ${page_count}\r\nPrezzo: ${(page_count - 1) * 0.15 + 0.4}`
                )}`
              );
              clearInterval(checkArrayValues);
            }
        }, 1000);
        sendDoc(file.name, fileType, reader.result);
      });
    }
  });
  input.click();
}
