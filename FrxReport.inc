<?php
// $Id$
/**
 * @file 
 * Basic report provider.  Controls the rendering of the report. 
 */
define('FRX_NS', 'urn:FrxReports'); 
require_once('FrxSyntaxEngine.inc'); 

class FrxReport { 
  public $blocks_loaded; 
  public $rpt_xml; 
  public $fields;
  public $category; 
  public $form; 
  public $access;  
  public $parameters; 
  public $options; 
  public $body;
  private $ids;
  private $cur_data; 
  private $output; 
  private $teng; 
  
  
  public function __construct($xhtml, $data=array()) {
    $this->teng = new FrxSyntaxEngine(FRX_TOKEN_EXP, '{}', $this);
    $this->access = array(); 
    $this->parameters = array(); 
    $this->options = array(); 
    if ($xhtml) { 
      if (!is_object($xhtml)) { 
        $this->rpt_xml = new SimpleXMLElement($xhtml);
      } 
      else {
        $this->rpt_xml = $xhtml; 
      }
      $this->cur_data = $data;
      
      // Load header data
      $rpt_xml = $this->rpt_xml; 
      $this->body = $rpt_xml->body;       
      if ($rpt_xml->head) {
        $this->title = (string)$rpt_xml->head->title; 
                
        foreach ($rpt_xml->head->children(FRX_NS) as $name => $node) { 
          switch ($name) { 
            case 'fields':               
              $this->fields = $node;
              break; 
            case 'category':              
              $this->category = (string)$node; 
              break;     
            case 'options':               
              foreach ($node->attributes() as $key => $value) { 
                $this->options[$key] = (string)$value; 
              }
              break; 
            case 'form':
              $this->form = (string) $value; 
              break; 
            case 'parameters':              
              foreach ($node->children(FRX_NS) as $key => $node) {
                $parm = array(); 
                foreach ($node->attributes() as $akey => $attr) {
                  $parm[$akey] = (string)$attr; 
                }
                $id = $parm['id']; 
                $parm['value']= (string)$node; 
                $this->parameters[$id] = $parm;                
              }
              break;
            case 'doctypes': 
              $this->doctypes = $value;
              break; 
               
          }
        }
      }
    }
  }
  
  /**
   * Get the data block 
   * @param $block
   * @return unknown_type
   */
  private function get_data($block, $clause='') {
    //@TODO: Merge xml data parameters into the report paramters        
    $this->cur_data = forena_invoke_data_provider($block, $this->cur_data, $clause); 
    if ($this->cur_data) $this->blocks_loaded = TRUE; 
  }
  
  
  /**
   * Recursive report renderer
   * Walks the nodes rendering the report. 
   */
  public function render_section(SimpleXMLElement $node) {     
    $cur_data = $this->cur_data;     
    $elements = count($node->xpath('*'));
    $continue = TRUE; 
    $frx = $node->attributes(FRX_NS);
    // Test to see if we have any nodes that are contains data url    
    if ($node->xpath('*//@frx:*') || $frx) {          
      $attrs = $node->attributes();
      $frx = $node->attributes(FRX_NS);
      $tag = $node->getName(); 
      if ((string)$frx['block']) {
        $this->get_data((string)$frx['block'], (string)$frx['clause']);           
        if (!$this->cur_data) $continue = FALSE;
      } 
      $attr_text=''; 
      
      if ($attrs) foreach ($attrs as $key => $value) { 
        $attr_text =  ' '. $key .'="'. (string)$value .'"';
      }
      

      if ((string)$frx['foreach'] ) {       
        // Save xml
        $path = (string)$frx['foreach'];        
        $data = $this->cur_data;   
              
        if ($data) $nodes = $data->xpath($path);
        $i=0;
        $tmp_attrs = (array)$attrs; 
                         
        if ($nodes) foreach ($nodes as $x) {
          $this->cur_data = $x;
          $i++; 
          $odd = $i & 1;
          $row_class = $odd ? 'odd':'even';  
          $tmp_attrs['class'] = trim($attrs['class'] .' '. $row_class); 
          $r_attr_text = ''; 
          foreach ($tmp_attrs as $key => $value) { 
             $r_attr_text .=  ' '. $key .'="'. (string)$value .'"';
          }
          $o .= $this->teng->replace('<'. $tag . $r_attr_text .'>', $cur_data);          
          foreach ($node->children() as $child) { 
            $o .= $this->render_section($child); 
          }
          $o .= '</'. $tag .'>';
        }
        
        $this->cur_data = $data; 
      } 
      elseif ($continue) { 

        $o .= $this->teng->replace('<'. $tag . $attr_text .'>', $this->cur_data);
        foreach ($node->children() as $child) {
          $o .= $this->render_section($child);  
        }
        $o .= '</'. $tag .'>';
      }
    } 
    else {       

      $tag = $node->getName(); 
      // We can render so lets do it. 
      $text = $node->asXML();      
      $o .= $this->teng->replace($text, $this->cur_data);
    }
    $this->cur_data = $cur_data;  
    return $o; 
  }
  
  /**
   * Render the report
   * @return unknown_type
   */
  public function render($format) {
    $rpt_xml = $this->rpt_xml;
        
    if ($rpt_xml->body) $rpt_xml = $rpt_xml->body; 
    $body_xml = $rpt_xml; 
  
    if ($body_xml && $body_xml->children()) foreach ($body_xml->children() as $node) {
      $o .= $this->render_section($node);   
    }
    
    return $o ;  
  }
  /*
   * Formatter used by the syntax engine to alter data that gets extracted.
   * This invokes the field translation  
   */
  public function format($value, $key, $data) {
    // Determine if there is a field overide entry    
    if ($this->fields) {
      $path = 'frx:field[@id="'. $key .'"]';
      $formatters = $this->fields->xpath($path); 
      if ($formatters) foreach ($formatters as $formatter) {        
        if (((string)$formatter['block'] == $this->block) || (!(string)$formatter['block'])) {
          
           //@TODO: Replace the default extraction with something that will get sub elements of the string
           $default = (string)$formatter; 
           $link = (string) $formatter['link']; 
           $format = (string) $formatter['format']; 
           $format_str = (string) $formatter['format-string'];            
        }  
      }
    }
    
    if ($format) { 
      $value = forena_format_data($value, $format, $format_str);
    }
    
    
    // Default if specified
    if (!$value && $default) { 
      $value = $default;  
    }
    
    if ($link) { 
      $link = $this->teng->replace($link,  $data, TRUE);
      list($url, $query) = explode('?', $link); 
      $value = l(htmlspecialchars_decode($value), $url, array('query' => $query));
    }
    return $value; 
  }
  
  /**
   * Delete a node based on id
   * @param unknown_type $id
   * @return unknown_type
   */
  public function deleteNode($id) {
    $path = '//*[@id="'. $id .'"]';
    $nodes = $this->rpt_xml->xpath($path);
    $node = $nodes[0];     
    $dom=dom_import_simplexml($node);
    $dom->parentNode->removeChild($dom);    
  }
  
  /**
   * Return the xml data for the report. 
   *
   * @return unknown
   */
  public function asXML() { 
     if ($this->rpt_xml) { 
       return $this->rpt_xml->asXML(); 
     } 
     else { 
       return '';    
     }
  }
  /**
   * Make sure all xml elements have ids
   */ 
  private function parse_ids() {
     $i=0;  
     if ($this->rpt_xml) {
       $this->rpt_xml->registerXPathNamespace('frx', FRX_NS);
       $frx_attributes = array();  
       $frx_nodes = $this->rpt_xml->xpath('body//*[@frx:*]');

       if ($frx_nodes) foreach ($frx_nodes as $node) {
          $attr_nodes = $node->attributes(FRX_NS);
          if ($attr_nodes) { 
            // Make sure every element has an id
            $i++;
            $id = 'forena-'. $i;  
  
            if (!(string)$node['id']) {
              $node->addAttribute('id', $id);
              //drupal_set_message('node '. $id. (string)$node['id']); 
            } 
            else { 
              if (strpos((string)$node['id'], 'forena-')===0) {
                // Reset the id to the numerically generated one 
                $node['id'] = $id;   
              } 
              else { 
                // Use the id of the element
                $id = (string)$node['id']; 
              }
            }
            
            // Save away the frx attributes in case we need them later. 
            $attr_nodes = $node->attributes(FRX_NS);
            $attrs = array(); 
            if ($attr_nodes) foreach ($attr_nodes as $key => $value) { 
              $attrs[$key] = (string)$value; 
            }
            // Save away the attributes
            $frx_attributes[$id] = $attrs; 
          }
       }

       $this->frx_attributes = $frx_attributes; 
     }
  }
  
  /**
   * Get the attributes by 
   *
   * @return array Attributes 
   * 
   * This function will return an array for all of the frx attributes defined in the report body
   * These attributes can be saved away and added back in later using. 
   */
  public function get_attributes_by_id() {
    $this->parse_ids();  
    return $this->frx_attributes; 
  }
  
  /**
   * Save attributes based on id match
   *
   * @param array $attributes
   * 
   * The attributes array should be of the form 
   * array( element_id => array( key1 => value1, key2 => value2)
   * The function restores the attributes based on the element id.   
   */
  public function save_attributes_by_id($attributes) {
    $rpt_xml = $this->rpt_xml;  
    if ($attributes) foreach ($attributes as $id => $att_list) { 
      $id_search_path = '//*[@id="'. $id . '"]'; 
      $fnd = $rpt_xml->xpath($id_search_path);
      if ($fnd) { 
        $node = $fnd[0]; 
   
        // Start attribute replacement
        $frx_attributes = $node->Attributes(FRX_NS); 
        
        foreach ($att_list as $key => $value) { 
          if (!$frx_attributes[$key]) { 
            $node['frx:'. $key] = $value; 
          } 
          else {
            unset($frx_attributes[$key]); 
            $node['frx:'. $key] = $value;
          }
        }
      }
    }
  }
  
  
}