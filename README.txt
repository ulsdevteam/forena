Forena Reports

Enterprise Reporting module that provides a mechanism for writing reports 
in a modified html syntax.  This module leverages XML, and for this 
reason required PHP 5.2 in order to run correctly. 

Installation
------------

Install in your sites/all/modules folder per the normal module installation 
method. 

Next visit the Site configuration->Forena page and configure the path where 
reports wil be saved.  This path should be in a place that is writable by your 
the user context which runs your web server.  When in doubt, specify the report
location within your files directory. 

After saving your settings, visit your my reports page to see help reports, 
sample reports, as well as some useful canned drupal administration reports. 
The provided sample reports also serve as documentation for the product. 


SVGGraph Installation
-----      
For SVG graphing support, Downlaod the latest version of SVGGraph from 

http://www.goat1000.com/svggraph.php

Move the SVGGraph folder into the sites/all/libraries folder of your site. 
You should be able to see a sites/all/libraries/SVGGraph/SVGGraph.php file when 
you are completed.

In the forena configuration menu (under site configuration) select the document 
types tab and enable the SVG document type.  

DataTables Installation 
-----
To provide dataTables support, Download dataTables from 

http://datatables.net

Extract and move the dataTables into your sites/all/libraries folder.

You should be able to see a sites/all/libraries/dataTables/media folder when you 
are complete. 

Author
------
David Metzler