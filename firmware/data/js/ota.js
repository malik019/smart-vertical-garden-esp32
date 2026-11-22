// 1. Lijst van alle bestanden op de ESP ophalen en tonen
function loadFileList() {
    fetch('/api/files')
        .then(function(res) { return res.json(); })
        .then(function(files) {
            var tbody = document.getElementById('file-list-body');
            if (!tbody) return;
            tbody.innerHTML = '';

            if (!files.length) {
                tbody.innerHTML = '<tr><td colspan="3">Geen bestanden gevonden.</td></tr>';
                return;
            }

            files.forEach(function(file) {
                var tr = document.createElement('tr');
                tr.innerHTML =
                    '<td>' + file.name + '</td>' +
                    '<td>' + file.size + ' B</td>' +
                    '<td>' +
                        '<a href="/api/file-download?file=' + encodeURIComponent(file.name) + '">' +
                            '<button type="button" class="btn">Download</button>' +
                        '</a> ' +
                        '<button type="button" class="btn btn-back" onclick="deleteFile(\'' + file.name + '\')">Verwijder</button>' +
                    '</td>';
                tbody.appendChild(tr);
            });
        })
        .catch(function() {
            var tbody = document.getElementById('file-list-body');
            if (tbody) {
                tbody.innerHTML = '<tr><td colspan="3">Fout bij ophalen bestandenlijst.</td></tr>';
            }
        });
}

// 2. Bestand verwijderen van de ESP (globaal bereikbaar voor onclick)
window.deleteFile = function(filePath) {
    if (!confirm('Weet je zeker dat je ' + filePath + ' wilt verwijderen?')) return;

    fetch('/api/file-delete?file=' + encodeURIComponent(filePath), { method: 'DELETE' })
        .then(function(res) {
            if (res.ok) {
                document.getElementById('file-progress').innerText = filePath + ' verwijderd.';
                loadFileList();
            }
        });
};

// 3. Initialiseer event listeners zodra de DOM volledig is geladen
document.addEventListener('DOMContentLoaded', () => {
    var uploadForm = document.getElementById('upload-form');
    if (uploadForm) {
        uploadForm.addEventListener('submit', function(e) {
            e.preventDefault();
            var form = e.target;
            var data = new FormData(form);
            var req = new XMLHttpRequest();
            
            req.upload.addEventListener('progress', function(event) {
                if (event.lengthComputable) {
                    var percent = Math.round((event.loaded / event.total) * 100);
                    document.getElementById('progress').innerText = "Uploaden: " + percent + "%";
                }
            });
            
            req.onload = function() {
                if (req.status === 200 && req.responseText.trim() === "OK") {
                    document.getElementById('progress').innerText = "Update geslaagd! ESP herstart nu...";
                    setTimeout(function() { window.location.href = '/'; }, 5000);
                } else {
                    document.getElementById('progress').innerText = "Update mislukt! Probeer het opnieuw.";
                }
            };
            
            req.open(form.method, form.action);
            req.send(data);
        });
    }

    var fileUploadForm = document.getElementById('file-upload-form');
    if (fileUploadForm) {
        fileUploadForm.addEventListener('submit', function(e) {
            e.preventDefault();
            var fileInput = document.getElementById('single-file-input');
            var file = fileInput.files.item(0);
            if (!file) return;

            var data = new FormData();
            data.append('file', file);

            var req = new XMLHttpRequest();
            req.upload.addEventListener('progress', function(event) {
                if (event.lengthComputable) {
                    var percent = Math.round((event.loaded / event.total) * 100);
                    document.getElementById('file-progress').innerText = "Bestand uploaden: " + percent + "%";
                }
            });

            req.onload = function() {
                if (req.status === 200) {
                    document.getElementById('file-progress').innerText = "Bestand succesvol opgeslagen in flash!";
                    fileInput.value = '';
                    loadFileList();
                } else {
                    document.getElementById('file-progress').innerText = "Upload van bestand mislukt!";
                }
            };

            req.open('POST', '/api/file-upload');
            req.send(data);
        });
    }

    // Laad de bestandenlijst zodra de pagina opent
    loadFileList();
});
