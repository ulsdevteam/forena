<?php
class FrxChart extends FrxRenderer {
	public function render() {
		// Get data from source
		$options = array();
		$series = array();
		$seriesx = array();
		$output;
		$data = array();
		$attributes = $this->mergedAttributes();
		$path = isset($attributes['xpath']) ? $attributes['xpath'] :  '*';
		$label = '';
		if (isset($attributes['options'])) {
		  parse_str($attributes['options'], $options);
		  $series = @$options['series'];
		  if (isset($options['seriesx'])) $seriesx = $options['seriesx'];
		  if (isset($options['label'])) $label =  $options['label'];
		}

		// Extract chart series data from xml.
		if ($series) {
			foreach((array)$series as $i=>$d) {
				if ($seriesx) {
					$data[] = $this->xmlToValues($path, $d, $seriesx[$i]);
				}
				else {
				  $data[] = $this->xmlToValues($path, $d, $label);
				}
			}

		}
		if ($data) {
			$graph = $this->graph();
			if (is_object($graph)) {
				$type = isset($attributes['type']) ? strtolower($attributes['type']) : 'bargraph';
				switch ($type) {
					case 'bargraph':
						if ($data)  $output = $graph->barGraph($data[0], $options);
					  break;
					case 'piechart':
						if ($data) $output = $graph->pieChart($data[0], $options);
						break;
					case 'stackedbargraph':
						if ($data) $output = $graph->stackedBarGraph($data, $options);
						break;
					case 'groupedbargraph':
						if ($data) $output = $graph->groupedBarGraph($data, $options);
						break;
					case 'linegraph':
						if ($data) $output = $graph->lineGraph($data[0], $options);
						break;
					case 'multilinegraph':
						if ($data) $output = $graph->multiLineGraph($data, $options);
						break;
					case 'scatterplot':
						if ($data) $output = $graph->scatterPlot($data, $options);
						break;
					case 'multiscatterplot':
						if ($data) $output = $graph->multiScatterPlot($data, $options);
						break;


				}
			}
		}

		return $output;
	}

	/**
	 * This is a placeholder function.  It really needs to be smart enough to pull the
	 * correct graph type depeding on the format sepcified.  This is likely to be based on
	 * document configuration set by the hosting
	 * Enter description here ...
	 */
	public function graph() {
		include_once 'FrxSVGGraph.inc';
		return FrxSVGGraph::instance(FrxReportGenerator::instance()->configuration('library_path'). '/SVGGraph');
	}

	public function get_value() {

	}

	public function xmlToValues($path, $data_path, $label_path) {
		$do = FrxData::instance();
		$data = $do->currentContext();
		$values = array();
		if (is_object($data)) {
			$nodes = $data->xpath($path);
			if ($nodes) foreach ($nodes as $i=>$node) {
				$do->push($node, $this->id);

				$val = floatval($this->teng->replace($data_path, TRUE));
				if ($label_path) {
					$key = $this->teng->replace($label_path, TRUE);
				}
				else {
					$key = $i;
				}
	      $values[$key] = $val;
				$do->pop();
			}
		}
    return $values;
	}
}