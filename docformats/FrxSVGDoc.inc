<?php
class FrxSVGDoc extends FrxDocument { 
  
  public function __construct() { 
    $this->content_type = 'image/svg+xml'; 
  }
  
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
  		$output .= $svg[0]->asXML();
  	} else {
  	  $output = '<svg/>'; 
  	}
  	return $output;
  }
}