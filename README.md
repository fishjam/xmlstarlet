This project form from [XMLStarlet 1.6.1](http://xmlstar.sourceforge.net/), and add a very important function(at least for me :) ),
It can insert a complex node value with ed(edit) command.

Example(add property into hadoop/core-site.xml file):

```
  xmlstarlet ed -s /configuration -t elem -n "" -v "<property><name>hadoop.tmp.dir</name><value>/usr/hadoop/tmp</value></property><property><name>fs.defaultFS</name><value>hdfs://localhost:9000</value></property>" hadoop/core-site.xml
```  
  
  Old xmlstarlet Result(The value is escaped):
```
<?xml version="1.0"?>
<?xml-stylesheet type="text/xsl" href="configuration.xsl"?>
<configuration>
<>&lt;property&gt;&lt;name&gt;hadoop.tmp.dir&lt;/name&gt;&lt;value&gt;/usr/hadoop/tmp&lt;/value&gt;&lt;/property&gt;&lt;property&gt;&lt;name&gt;fs.defaultFS&lt;/name&gt;&lt;value&gt;hdfs://localhost:9000&lt;/value&gt;&lt;/property&gt;</></configuration>
```  

New xmlstarlet Result(This is what I wanted):
```
<?xml version="1.0"?>
<?xml-stylesheet type="text/xsl" href="configuration.xsl"?>
<configuration>
<property><name>hadoop.tmp.dir</name><value>/usr/hadoop/tmp</value></property><property><name>fs.defaultFS</name><value>hdfs://localhost:9000</value></property></configuration>
```

  
XMLStarlet is a command line XML toolkit which can be used to transform, 
query, validate, and edit XML documents and files using  simple set of shell 
commands in similar way it is done for plain text files  using grep/sed/awk/
tr/diff/patch.

