<?php
/**
 * File class for loading raw xml files.  
 * This class is used as a reference implementation of a data engine, but is 
 * also used to provide sample data files and reports. 
 * @author metzlerd
 *
 */
class FrxFileEngine {
	private $path; 
	public function __construct($uri) { 
		list($protocol,$path) = explode(':', $uri, 2); 
		$this->path = $path;  
	}
	
	public function getName() {
		return 'XML File';
	}
	
	public function block($blockName, $parm_data, $subQuery) {
		$xmlData = file_get_contents($this->path. '/' . $blockName . '.xml', FILE_USE_INCLUDE_PATH ); 
		$xml = new SimpleXMLElement($xmlData); 
		
		//@TODO: Implment xpath subquery parameter
		// Should filter returned nodes, but within the original root element? 
	}
	
}