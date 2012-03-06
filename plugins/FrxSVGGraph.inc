<?php
class FrxSVGGraph {
	private $graph;

	static function instance($directory) {
		static $inst='';
		if (!$inst) {
			$library = rtrim($directory,'/') . '/SVGGraph.php';
			if (file_exists($library)) {
			  require_once $library;
			  $inst = new FrxSVGGraph();
			}
		}
		return $inst;
	}

	function __construct($width = 640, $heigth = 480) {
		$this->graph = new SVGGraph($width, $heigth);
	}

  // Generate a bar graph.
	function barGraph($data, $options) {
    if (@$options['style']=='3D') {
      $type = 'Bar3DGraph';
    }
    elseif (@$options['style']=='horizontal') {
    	$type = 'HorizontalBarGraph';
    }
    else {
      $type = 'BarGraph';
    }
    return $this->render($type, $options, $data);
	}

	function stackedBarGraph($data, $options) {
		if (@$options['style'] == 'horizontal') {
			$type = 'HorizontalStackedBarGraph';
		}
		else {
			$type = 'StackedBarGraph';
		}
		return $this->render($type, $options, $data);
	}

	function groupedBarGraph($data, $options) {
		if (@$options['style'] == 'horizontal') {
			$type = 'HorizontalGroupedBarGraph';
		}
		else {
			$type = 'GroupedBarGraph';
		}
		return $this->render($type, $options, $data);
	}

	// Generate a pie chart
	function pieChart($data, $options) {
    if (@$options['style']=='3D') {
      $type = 'Pie3DGraph';
    }
    else {
      $type = 'PieGraph';
    }
    return $this->render($type, $options, $data);
	}

	function lineGraph($data, $options) {
		return $this->render('LineGraph', $options, $data);
	}

	function multiLineGraph($data, $options) {
		return $this->render('MultiLineGraph', $options, $data);
	}

	// Scatter plot
	function scatterPlot($data, $options) {
		return $this->render('ScatterGraph', $options, $data);
	}

	// Multi scatter plot
	function multiScatterPlot($data, $options) {
		return $this->render('MultiScatterGraph', $options, $data);
	}

	function render($type, $options, $data) {
    $this->graph->Values($data);
    if (is_array(@$options['colors'])) {
      $this->graph->colours = $options['colors'];
    }
		ob_start();
		$this->graph->Render($type, FALSE);
		$output = ob_get_contents();
		ob_end_clean();
		return $output;
	}


}