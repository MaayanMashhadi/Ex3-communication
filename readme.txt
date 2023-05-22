api:
GET: http://127.0.0.1:8000/index.txt?lang=
for example: http://127.0.0.1:8000/index.txt?lang=he

PUT: http://127.0.0.1:8000/index.txt?lang=''&title=''
for example: http://127.0.0.1:8000/index.txt?lang='en'&title='hi'
(this is changing the title of the page- put 'hi' string)

DELETE: http://127.0.0.1:8000/index.txt
(this will delete the txt file)

HEAD: http://127.0.0.1:8000/index.txt
(will gives the metadata of the file txt)