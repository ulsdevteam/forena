<?php
/**
 * File class for loading raw xml files.  
 * This class is used as a reference implementation of a data engine, but is 
 * also used to provide sample data files and reports. 
 * @author metzlerd
 *
 */
class FrxFileEngine extends FrxDataEngine {
	private $path; 
	public function __construct($conf, $repos_path) { 

		list($protocol,$path) = explode('://', $conf['uri'], 2); 
		if (!$path) $path = $protocol; 
		
		$this->path = $path;  
	}
	
	public function getName() {
		return 'XML File';
	}
	
	public function data($block_name, $parm_data='', $subQuery='') {

		$xmlData = file_get_contents($this->path. '/' . $block_name . '.xml', FILE_USE_INCLUDE_PATH ); 
		if ($xmlData) { 
  		try { 
  	   	$xml = new SimpleXMLElement($xmlData);
  		} catch (Exception $e) { 
  		  print "Error processing xml\n";
  		  print $e->getMessage();  
  		  print $xmlData; 
  		}
		}
	
		//@TODO: Implment xpath subquery parameter
		// Should filter returned nodes, but within the original root element? 
		return $xml; 
	}
	
}