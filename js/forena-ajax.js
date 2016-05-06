/**
 * @file
 *
 * Forena ajax library.  Includes useful functions for performing ajax replacements on reports.
 * by imre.horan
 */
(function ($) {
  // Standard AJAX error handler.
  /**
   * Handler for ajax error responses.
   *
   */
  Drupal.ajax.prototype.error = function(response, uri) {
    // Suppressing the error message if status = 0.
    if (response.status != 0) {
      alert(Drupal.ajaxError(response, uri));
    }
    // Remove the progress element.
    if (this.progress.element) {
      $(this.progress.element).remove();
    }
    if (this.progress.object) {
      this.progress.object.stopMonitoring();
    }
    // Undo hide.
    $(this.wrapper).show();
    // Re-enable the element.
    $(this.element).removeClass('progress-disabled').removeAttr('disabled');
    // Reattach behaviors, if they were detached in beforeSerialize().
    if (this.form) {
      var settings = response.settings || this.settings || Drupal.settings;
      Drupal.attachBehaviors(this.form, settings);
    }
  };

  $.fn.forenaAutoload = function () {
    this.hide();
    this.addClass(".forena-autoload-processed");
    this.click();
  }

  // jQuery plugin for adding a select class to element
  $.fn.forenaSelect = function(selector) {
    $('.selected', this).removeClass('selected');
    $(selector, this).addClass('selected');
  };
  
  Drupal.behaviors.forenaAjax = {
    attach: function (context, settings) {
      // Auto click the reports for ajax.
      $('.use-ajax.ajax-processed.forena-autoload:not(.forena-autoload-processed', context).forenaAutoload();
    }
  };

})(jQuery);

