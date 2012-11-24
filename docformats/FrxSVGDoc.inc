<?php
class FrxSVGDoc extends FrxDocument { 
  public function render($r, $format, $options = array()) { 
    $output = '';
  	$doc = new DomDocument();
  	$xml_body = '<html><body>'. $r->html . '</body></html>';
  	$doc->formatOutput=FALSE;
  	$doc->strictErrorChecking = FALSE;
  	$doc->loadXML($xml_body);
  	//print($xml_body);
  	$xml = simplexml_import_dom($doc);
  	$xml->registerXPathNamespace('__empty_ns', 'http://www.w3.org/2000/svg');
  	if ($xml) $svg = $xml->xpath('//__empty_ns:svg');
  	if ($svg) {
   	  $output = '<?xml version="1.0" encoding="UTF-8" standalone="no" ?>';
  		$output .= $svg[0]->asXML();
  	} else {
  	  $output = '<svg/>'; 
  	}
  	header('Content-Type: image/svg+xml');
    header('Cache-Control:');
    header('Pragma:');
    header('Cache-Control: must-revalidate');
  	print $output;
  }
}