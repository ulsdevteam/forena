<?php
class FrxInclude extends FrxRenderer {
	public function render() {
		// Get data from source
		$attributes = $this->mergedAttributes();
		$output = '';

		// Determine data type
		$include = @$attributes['src'];
		$include = $this->teng->replace($include);
		@list($url, $query_str)=@explode('?', $include);
    $report_url = $url;
		$parts = @explode('/', $url);
		$file = @$parts[count($parts) - 1];
		$parts = explode('.', $file);
		// Determine file extention
		$ext = count($parts) > 1 ? $parts[count($parts) - 1] : '';
		$query = array();
		parse_str($query_str, $query );
    $options = array('query' => $query);
		$url = url($url, $options);

    if ($this->format=='web') {
      $output = $this->render_reference($url, $ext, $attributes);
    }
		else {
			$output = $this->renderInclude(str_replace('reports/','',$report_url), $query);
		}
		return $output;
	}

	public function renderInclude($name, $parms) {
	  $desc= Frx::Menu()->parseURL($name);
	  $name = $desc['name'];
	  Frx::Data()->push($parms, 'parms');
	  print_r($parms);
	  $r = @FrxReportGenerator::instance()->get_report($name, $parms);
    if (!$r || !$r->rpt_xml) {
    	return '';
    }

    //check for default parameters
    $r->processParameters();
    $output = $r->render($desc['format']);

    Frx::Data()->pop();
    return $output;
	}


  function render_reference($url, $ext, $attributes) {
  	$ext = strtolower($ext);
  	$attributes = $this->teng->replace($attributes);
  	switch($ext) {
  		case 'png':
  		case 'gif':
  		case 'jpg':
  		case 'jpeg':
      	$x = new SimpleXMLElement('<img/>');
      	$x['src'] = $url;
      	if (isset($attributes['height'])) $x['height'] = $attributes['height'];
      	if (isset($attributes['width'])) $x['width'] = $attributes['width'];
  		  break;
  		case 'svg':
  			$x = new SimpleXMLElement('<embed/>');
  			$x['src'] = $url;
  			$x['type'] = 'image/svg+xml';
    		$x['pluginspage'] = "http://www.adobe.com/svg/viewer/install/";
      	if (isset($attributes['height'])) $x['height'] = $attributes['height'];
      	if (isset($attributes['width'])) $x['width'] = $attributes['width'];
  			break;
  		default:
  			$x = new SimpleXMLElement('<a>' . $ext . ' document</a>' );
  			$x['href'] = $url;
  	}

  	if (isset($attributes['id'])) $x['id'] = $attributes['id'];
  	return $x->asXML();
  }


}