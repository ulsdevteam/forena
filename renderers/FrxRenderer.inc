<?php
/**
 * @file FrxRenderer.inc
 * Base class for Frx custom renderers
 * @author davidmetzler
 *
 */
class FrxRenderer {
  public $teng;           // Token replacement engine.
  public $reportDocDomNode; //A Dom node version of the element.  This is important if you want to walk text nodes.
  public $reportDocNode;  // SimpleXML Report Document Node -- The node of the report we are rendering
  public $frxAttributes;  // Frx Attributes of the node we are rendering.
  public $htmlAttributes;  // Html attributes of the node that we are rendering
  public $dataProvider;   // An FrxData instance that provides the data assiated with the report.
  public $name;
  public $id;
  public $format;
  public $frxReport;  // The report object being used.
  public $columns;
  public $numeric_columns;
  public $xmlns = 'urn:FrxReports';


  public function initReportNode(DOMElement $domNode, FrxReport $frxReport) {
    $this->dom = $frxReport->dom;
    $this->reportDocDomNode = $domNode;
    $this->dataProvider = Frx::Data();
    $this->reportDocNode = $node = simplexml_import_dom($domNode);
    $this->teng = $frxReport->teng;
    $this->frxReport = $frxReport;
    $this->format = $this->frxReport->format;
    $skin = Frx::Data()->getContext('skin');
    $this->settings = isset($skin['FrxReport']) ? $skin['FrxReport'] : array();
    $this->reportDocNode->getName();
    $this->htmlAttributes = $node->attributes();
    $this->id = (string)$this->htmlAttributes['id'];
    $this->frxAttributes = $node->attributes(FRX_NS);
  }

  public function replaceTokens($text, $raw_mode=FALSE) {
    return $this->teng->replace($text, $raw_mode);
  }


  /**
   * Returns true if a node has no children, no attributes (or empty values
   * in it's attributes), and no text content
   */
  protected function _check_xml_node_empty($node) {
    if (!count($node->children())) {
      $empty_attributes = false;
      if (count($node->attributes()) > 0) {
        foreach ($node->attributes() as $attr => $attr_val) {
          $num_total_attr = 0;
          $num_empty_attr = 0;
          $attr_val = (string)$attr_val;
          if ($attr_val == "") {
            ++$num_empty_attr;
            //return '';
          }
          ++$num_total_attr;
        }
        if ($num_total_attr == $num_empty_attr) {
          $empty_attributes = true;
        }
      } else {
        $empty_attributes = true;
      }
      if ($empty_attributes) {
        $node_text = dom_import_simplexml($node)->textContent;
        if ($node_text == "") {
          // empty xml element
          return true;
        }
      }
    }
    return false;
  }

  /**
   * Recursive report renderer
   * Walks the nodes rendering the report.
   */
  public function renderDomNode(DOMNode $dom_node) {
    $continue = TRUE;
    $is_data_block = FALSE;
    $o = '';
    $node_type = $dom_node->nodeType;
    $settings = $this->settings;
    $context = Frx::Data()->currentContextArray();

    // Shortcut process a text node
    if ($node_type == XML_TEXT_NODE|| $node_type == XML_ENTITY_REF_NODE || $node_type == XML_ENTITY_NODE)
    {
      $text = $dom_node->textContent;
      $o .= $this->teng->replace($text);
      return $o;
    }

    //Handle comment nodes
    if ($node_type == XML_COMMENT_NODE) {
      if (!empty($dom_node->length) &&
      !empty($dom_node->data)) {
        $text = $dom_node->data;
        // strip empty comments if configured to
        if (!empty($settings['stripEmptyComments'])) {
          $comment_text = trim($this->teng->replace($text));
          if ($comment_text === '') {
            return '';
          }
        }
        // comment markup is stripped so need to add it back in
        $o .= '<!--' . $this->teng->replace($text) . '-->';
        return $o;
      } else {
        return '';
      }
    }

    // Continue processing non text nodes
    $node = simplexml_import_dom($dom_node);
    // Special catch to make sure we don't process bad nodes
    if (!is_object($node)) {
      return '';
    }

    $frx = $node->attributes(FRX_NS);
    $include_root = !isset($frx['skip_root']) || !$frx['skip_root'];
    $elements = $dom_node->childNodes->length;

    // Check for invalid link processing.
    if (@(string)$frx['invalid_link']) {
      $old_link_mode = $this->link_mode;
      $this->frxReport->link_mode = (string)$frx['invalid_link'];
    }

    // Test to see if we have any nodes that contain data url
    if ($node->xpath('*//@frx:*') || $frx) {
      $attrs = $node->attributes();
      $id = (string)$attrs['id'];
      $frx = $node->attributes(FRX_NS);
      $tag = $node->getName();

      if ($this->frxReport->preview_mode) $o .= Frx::Editor()->editLinks($frx, $id, $context);


      if ((string)$frx['block']) {
        $is_data_block = TRUE;
        $xml = $this->frxReport->getData((string)$frx['block'], (string)$frx['clause'], $id, (string)$frx['parameters']);
        if ($xml) {
          Frx::Data()->push($xml, $id);
        }
        else {
          return $o;
        }
      }

      //Implment if then logic
      if ((string)$frx['if']) {
        $cond = (string)$frx['if'];
        if (!$this->teng->test($cond)) return '';
      }

      // Preserve non frx attributes
      $attr_text='';
      $tmp_attrs = array();
      if ($attrs) foreach ($attrs as $key => $value) {
        $attr_text .=  ' ' . $key . '="' . (string)$value . '"';
        $tmp_attrs[$key] = (string)$value;
      }

      // Determine if we have a custom renderer
      $renderer = (string)$frx['renderer'];
      // if we have a foreach in this node, we need to iterate the children
      if ((string)$frx['foreach'] ) {
        // Save xml
        $path = $this->teng->replace((string)$frx['foreach'], TRUE);
        $data = Frx::Data()->currentContext();

        if (is_object($data)) {
          if (method_exists($data, 'xpath')) {
            $nodes = $data->xpath($path);
          }
          else {
            $nodes = $data;
          }
        }
        else {
          $nodes = (array)$data;
        }

        // Sort values
        $sort = @(string)$frx['sort'];
        if ($sort) {
          $compare_type = @(string)$frx['compare'];
          $this->frxReport->sort($data, $sort, $compare_type);
        }

        // Group values
        $group = @(string)$frx['group'];
        if ($group) {
          $nodes = $this->frxReport->group($nodes, $group);
        }

        $i=0;

        //$tmp_attrs = (array)$attrs;
        if ($nodes) foreach ($nodes as $x) {
          if ($group) {
            Frx::Data()->setContext('group', $x[0]);
          }
          Frx::Data()->push($x, $id);
          $i++;
          $odd = $i & 1;
          $row_class = $odd ? 'odd' : 'even';
          $r_attr_text = '';
          if (isset($attrs['id'])) {
            if (strpos($attrs['id'],'{')!== FALSE) {
              $id = $this->teng->replace($attrs['id']);
            }
            else {
              if (!empty($settings['numericFrxForeachID'])) {
                $id_attr = $i;
              } else {
                $id_attr = $attrs['id'] . '-' . $i;
              }
            }
            $tmp_attrs['id'] =  $id_attr;
          }

          if (@!$settings['noHelperClasses']) {
            $tmp_attrs['class'] = trim($attrs['class'] . ' ' . $row_class);
          }

          foreach ($tmp_attrs as $key => $value) {
            $r_attr_text .=  ' ' . $key . '="' . (string)$value . '"';
          }

          if ($include_root) $o .= $this->teng->replace('<' . $tag . $r_attr_text . '>');
          foreach ($dom_node->childNodes as $child) {
            $o .= $this->renderDomNode($child);
          }
          if ($include_root) $o .= '</' . $tag . '>';
          Frx::Data()->pop();
        }
      }
      elseif ($continue) {
        if ($renderer) {
          // Implement custom renderer.
          $co = Frx::Controls($renderer);
          if ($co) {
            $co->initReportNode($dom_node, $this->frxReport);
            $o = $co->render();

          }
        }
        else {
          if ($include_root) $o .= $this->teng->replace('<' . $tag . $attr_text . '>');

          // None found, so render children
          foreach ($dom_node->childNodes as $child) {
            $o .= $this->renderDomNode($child);
          }
          if ($include_root) $o .= '</' . $tag . '>';
        }
      }
      if ($is_data_block && $continue) {
        Frx::Data()->pop();
      }
    }
    else {
      // We can render so lets do it.
      $text = $node->asXML();
      $node_xml = $this->teng->replace($text);

      // Strip out empty leaf nodes if report is configured to
      if (!empty($settings['stripEmptyElements'])) {
        $node_updated = new SimpleXMLElement($node_xml);
        if ($this->_check_xml_node_empty($node_updated)) {
          return '';
        }
      }
      $o .= $this->teng->replace($text);
    }

    // Restore link processing.
    if (@(string)$frx['invalid_link']) {
      $this->frxReport->link_mode = $old_link_mode;
    }

    return $o;
  }

  public function renderChildren(DOMNode $domNode) {
    $o = '';
    foreach ($domNode->childNodes as $node) {
      $o .= $this->renderDomNode($node);
    }
    return $o;
  }

  public function render() {
    return $this->renderDomNode($this->reportDocDomNode);
  }

  public static function addAttributes(&$attributes, $key, $value) {
    $parts = explode('_', $key);
    $suff = '';
    if (count($parts) > 1) {
      $suff=array_pop($parts);
      $part = implode('_', $parts);
    }

    // If we have _0 _1 _2 attributes convert them into arrays.
    if ((int)$suff || $suff === '0') {
      $attributes[$part][] = (string)$value;
    }
    else {
      $attributes[$key] = (string)$value;
    }
  }

  public function resetTemplate() {
     $node = $this->reportDocDomNode;
     $this->removeChildren($node);
     $tag = $node->tagName;
     $new_node = $this->dom->createElement($tag);
     $this->frxAttributes = array();
     $parent = $node->parentNode;
     $parent->replaceChild($new_node, $node);
     $this->reportDocDomNode = $new_node;
     $this->initReportNode($new_node, $this->frxReport);
     return $node;
  }



  /**
   * Set FRX attributes.
   * @param DOMNode $node
   * @param unknown $attributes
   * @param unknown $frxattributes
   */
  public function setAttributes(DOMElement $node, $attributes, $frx_attributes) {
    if ($attributes) foreach ($attributes as $key => $value) {

      $node->setAttribute($key, $value);

    }

    // Iterate the value
    if ($frx_attributes) foreach ($frx_attributes as $key => $value) {

      // If the value is an array create multiple attributes
      // that are of the form key_1, key_2 .... etc.
      if (is_array($value)) {
        $i=0;
        $done=FALSE;
        while(!$done) {
          $v = '';
          if ($value) $v = array_shift($value);
          $i++;
          $k = $key . '_' . trim((string)$i);
          $node->setAttribute($k,$v);
          if (!$v) {
            $done = TRUE;
          }
        }
      }
      // A normal value.
      else {
        if ($value) $node->setAttributeNS($this->xmlns, $key, $value);
      }
    }

  }

  /**
   * Standard php array containing merged attributes
   * Enter description here ...
   */
  public function mergedAttributes() {
    $attributes = array();
    if (isset($this->frxAttributes)) foreach ($this->frxAttributes as $key => $data) {
      FrxRenderer::addAttributes($attributes, $key, $data);
    }
    if (isset($this->htmlAttributes)) foreach ($this->htmlAttributes as $key => $data) {
      FrxRenderer::addAttributes($attributes, $key, $data);
    }
    $skin_data = Frx::Data()->getContext('skin');
    $class = get_class($this);

    if (isset($skin_data[$class])) {
      $attributes = array_merge($skin_data[$class], $attributes);
    }
    $classes = class_parents($this);
    array_pop($classes);
    if ($classes) foreach ($classes as $class) {
      if (isset($skin_data[$class])) {
        $attributes = array_merge($attributes, $skin_data[$class]);
      }
    }
    return $attributes;
  }

  public function replacedAttributes() {
    $attributes = array();
    if (isset($this->frxAttributes)) foreach ($this->frxAttributes as $key => $data) {
      $attributes[$key] =  $this->teng->replace((string)$data, TRUE);
    }
    if (isset($this->htmlAttributes)) foreach ($this->htmlAttributes as $key => $data) {
      $attributes[$key] = $this->teng->replace((string)$data, TRUE);
    }
    return $attributes;
  }

  /**
   * Return the inside xml of the current node
   *
   */
  public function innerXML() {
    $xml = $this->reportDocNode;
    $tag = $xml->getName();
    $text = '';
    if (is_object($xml) && is_object($xml->$tag)) {
      $text = $xml->asXML();
      $text = preg_replace("/<\/?" . $tag . "(.|\s)*?>/", "", $text);
    }
    return $text;
  }

  /**
   * Render a drupal form in a forena template
   * @param $form array
   */
  public function drupalRender($form) {
    $output = drupal_render($form);
    return $output;
  }

  /**
   * Returns the section
   * Enter description here ...
   */
  public function configForm($config, $xml='') {
    $form_ctl = array();

    $form_ctl['heading'] = array(
        '#type' => 'textfield',
        '#title' => t('Heading'),
        '#default_value' => @$config['heading'],
    );

    $form_ctl['description'] = array(
        '#type' => 'textarea',
        '#title' => t('Description'),
        '#rows' => 2,
        '#default_value' => @$config['description'],
        '#ajax' => $this->configAjax('blur')
    );

    return $form_ctl;
  }

  public function configAjax($event='') {
    $ajax = array(
        'callback' => 'forena_template_callback',
        'wrapper' => 'forena-template-wrapper',
      );
    if ($event) $ajax['event'] = $event;
    return $ajax;
  }

  function addNode($cur_node, $indent, $tag='div', $value='', $attributes=array(), $frx_attributes=array()) {
    $dom = $this->dom;
    if (!$cur_node) {
      return;
    }

    if ($indent) {
      $tnode = $dom->createTextNode("\n" . str_repeat(' ', $indent));
      $cur_node->appendChild($tnode);
    }
    $node = $this->dom->createElement($tag, $value);
    $cur_node->appendChild($node);
    $this->setAttributes($node, $attributes, $frx_attributes);
    $cur_node->appendChild($this->dom->createTextNode(""));
    return $node;
  }

  public function columns($xml, $path='/*/*') {
    //create an array of columns
    if (!is_object($xml)) return array();
    $rows = $xml->xpath($path);
    $column_array = array();
    $numeric_columns = array();
    foreach ($rows as $columns) {
      foreach ($columns as $name => $value) {
        $label = str_replace('_', ' ', $name);
        $column_array[$name] = $label;
        if (is_numeric((string)$value)) {
          $numeric_columns[$name] = $label;
        }
        else {
          if (isset($numeric_columns[$name])) unset($numeric_columns[$name]);
        }
      }
      if (is_object($xml) && method_exists($xml, 'attributes')) {
        foreach ($xml->attributes() as $name => $value) {
          $column_array['@' . $name] = '@' . $name;
        }
      }
    }
    $this->columns = $column_array;
    $this->numeric_columns = $numeric_columns;
    return $column_array;
  }

  function addText($cur_node, $text) {
    $dom = $this->dom;
    $tnode = $dom->createTextNode($text);
    $cur_node->appendChild($tnode);
    return $tnode;
  }

  /**
   *
   * Extract a configuration var removing it from the array
   * @param string $key attribute key for the data being extracted.
   * @param array $config
   */
  public function extract($key, &$config) {
    $value = '';
    if (isset($config[$key])) {
      $value = $config[$key];
      unset($config[$key]);
    }
    return $value;
  }

  /**
   *
   * Generate generic div tag.
   * @param unknown_type $config
   * @param unknown_type $text
   */
  public function blockDiv(&$config, $text='') {
    $node = $this->reportDocDomNode;
    $heading = $this->extract('heading', $config);
    $descr = $this->extract('description', $config);
    $include = $this->extract('include', $config);
    $block = $this->extract('block', $config);
    $foreach = $this->extract('foreach', $config);
    $id = $this->extract('id', $config);
    if (!$id) {
      $id = $this->idFromBlock($block);
    }
    $class = $this->extract('class', $config);
    $frx_attributes = array(
        'block' => $block,
    );
    if ($foreach) $frx_attributes['foreach'] = $foreach;
    $attributes = array(
        'id' => $id,
        'class' => $class,
    );

    $this->setAttributes($node, $attributes, $frx_attributes);
    if ($heading) {
      $this->addNode($node, 4, 'h2', $heading);
    }
    if ($descr) {
      $this->addNode($node, 4, 'p', $descr);
    }
    if ($include) {
      $src = 'reports/' . str_replace('/', '.', $include);
      $this->addNode($node, 4, 'div', NULL, NULL, array('renderer' => get_class($this), 'src' => $src));
    }

    return $node;
  }

  /**
   *
   * Enter description here ...
   * @param string $data_block
   * @param SimpleXMLElement $xml
   * @param array $config
   */
  public function generate($xml , $config) {


    if (!@$config['foreach']) $config['foreach']='*';


    $columns = $this->columns($xml);
    $text = '';
    if ($columns) foreach ($columns as $col => $label) {
      $text .= ' {' . $col . '}';
    }
    $this->blockDiv($config, $text);

  }

  public function template() {
    $body = $this->body;
    $output = '';
    foreach ($body->childNodes as $node) {
      $output .= $this->dom->saveXML($node);
    }
    return $output;
  }

  public function asXML() {
    return $this->dom->saveXML();
  }

  /**
   * Simple function to get id from node.
   * @param unknown $block
   * @return mixed
   */
  public function idFromBlock($block) {
    $parts = explode('/', $block);
    $id = str_replace('.', '_', array_pop($parts));
    return $id;
  }

  /**
   * Sets the first child element to a node and returns it.
   * IF the node
   * @param DOMNode $node
   * @param unknown $tag
   * @param unknown $indent
   * @return Ambigous <string, void, DOMElement>
   */
  public function setFirstNode(DOMElement $parent_node,  $indent=0, $tag='div',  $value='', $attributes=array(), $frx_attributes=array()) {
    $dom = $this->dom;
    if (!$parent_node) {
      return;
    }
    $nodes = $parent_node->getElementsByTagName($tag);
    if ($nodes->length) {
      $node = $nodes->item(0);
      $this->setAttributes($node, $attributes, $frx_attributes);
    }
    else {
      $node = $this->addNode($parent_node, $indent, $tag, $value, $attributes, $frx_attributes);
    }
    return $node;
  }

  public function removeChildren(DOMNode $node) {
    while (isset($node->firstChild)) {
      $this->removeChildren($node->firstChild);
      $node->removeChild($node->firstChild);
    }
  }

  public function xmlToValues($path, $data_path, $label_path='', $pairs = FALSE) {
    $do = Frx::Data();
    $data = $do->currentContext();
    $values = array();
    if (is_object($data)) {
      $nodes = $data->xpath($path);
      if ($nodes) foreach ($nodes as $i => $node) {
        $do->push($node, $this->id);

        $val = $this->teng->replace($data_path, TRUE);
        if ($label_path) {
          $key = strip_tags($this->teng->replace($label_path, FALSE));
        }
        else {
          $key = $i;
        }
        if ($pairs && $label_path) {
          $values[] = array(floatval($key), floatval($val));
        }
        else {
          $values[$key] = $val;
        }


        $do->pop();
      }
    }
    return $values;
  }

}