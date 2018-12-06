(function () {
  'use strict';
  var gulp = require("gulp");
  var ts = require('gulp-typescript');
  var sourcemaps = require('gulp-sourcemaps');
  var tslint = require("gulp-tslint");
  var minify = require('gulp-minify');
  var webpack = require('webpack-stream');

  gulp.task("build", function build () {
      return gulp.src([
              "src/**/*.ts",
              "microsoft.cognitiveservices.speech.sdk.ts"],
              {base: '.'})
          .pipe(tslint({
        formatter: "prose",
              configuration: "tslint.json"
      }))
      .pipe(tslint.report({
              summarizeFailureOutput: true
          }))
          .pipe(sourcemaps.init())
          .pipe(ts({
              target: "ES5",
              declaration: true,
              noImplicitAny: true,
              removeComments: false,
              outDir: 'distrib/lib'
          }))
          .pipe(sourcemaps.write('.'))
          .pipe(gulp.dest('distrib/lib'));
  });

  gulp.task("bundle", gulp.series("build", function bundle () {
      return gulp.src('bundleApp.js')
      .pipe(webpack({
          output: {filename: 'microsoft.cognitiveservices.speech.sdk.bundle.js'},
          devtool: 'source-map',
          module:  {
              rules: [{
                     enforce: 'pre',
                     test: /\.js$/,
                     loader: "source-map-loader"
              }]
          }
      }))
      .pipe(gulp.dest('distrib/browser'));
  }));
}());
