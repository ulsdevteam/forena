<?php
require_once('FrxSyntaxEngine.inc'); 

class FrxReport { 
	private $rpt_xml; 
	private $cur_xml; 
	private $output; 
	private $data_context; 
	private $teng; 
	
	public function __construct($xhtml) {
	  $this->teng = new FrxSyntaxEngine(FRX_TOKEN_EXP); 
	  if (!is_object($xhtml)) { 
	  	$this->rpt_xml = new SimpleXMLElement($xhtml); 	
	  } else { 
	  	$this->rpt_xml = $xhtml; 
	  }
	}
		
	/**
	 * Get the data block 
	 * @param $block
	 * @return unknown_type
	 */
	private function get_data($block) {
	  //@TODO: Merge xml data parameters into the report paramters
	  $this->cur_xml = forena_invoke_data_engine($block, $parameters); 
	}
	
	private function process_frx_attributes(SimpleXMLElement $node) { 
	    $attrs = $node->attributes('urn:FrxReports'); 
      if ($attrs) foreach ($attrs as $key => $value) { 
          switch ($key) {
          	case "block":
               $this->get_data((string)$value);
               break;  
        }
      }		
	}
	
	
    /**
     * Recursive report renderer
     * Walks the nodes renering the report. 
     */
	private function render_section(SimpleXMLElement $node) {

	$text_nodes = count($node->xpath('child::text()')); 
	$child_nodes = count($node->xpath('.//*')); 
  $elements = count($node->xpath('*')); 
   // Test to see if we have any nodes that are contains data url
		if ($node->xpath('*//@frx:*')||$node->xpath('./@frx:*')) {
		  $attrs = $node->attributes(); 
		  $tag = $node->getName(); 
		  $this->process_frx_attributes($node); 
		  if ($attrs) foreach ($attrs as $key => $value) { 
		    $attr_text =  ' ' .$key. '="'. (string)$value .'"';
		  }
      
	
	  	$frx = $node->attributes('urn:FrxReports');
		  
		  if ((string)$frx['foreach'] ){
		  	// Save xml
		  	$path = (string)$frx['foreach']; 
		  	$data = $this->cur_xml; 
		  	if($data)	$nodes = $data->xpath($path); 
		  	if ($nodes) foreach ($nodes as $x) {
		  		$this->cur_xml = $x;
		  		
		  		$o .= '<'. $tag. $attr_text . '>';
		
		  		foreach($node->xpath('child::node()') as $child) { 
		  			$o .= $this->render_section($child); 
		  		}
		  		$o .= '</'. $tag .'>';

		  	
		  	}
		  	$this->cur_xml = $data; 
		  	
		  } else {
		  	$o .= '<'. $tag. $attr_text . '>';
			  foreach ($node as $child) {
			    $o.= $this->render_section($child); 	    
			  }
        $o .= '</'. $tag .'>'; 
		  }
		} else { 
      $tag = $node->getName(); 
			// We can render so lets do it. 
			$text = $node->asXML(); ; 
			$o.=$this->teng->replace($text,$this->cur_xml);
		}	
		return $o; 
	}
	
	/**
	 * Render the report
	 * @return unknown_type
	 */
	public function render() { 
	  $rpt_xml = $this->rpt_xml;
	  return $this->render_section($rpt_xml);  
	}
}