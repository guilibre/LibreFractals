// @ts-check
const eslint = require("@eslint/js");
const tsEslint = require("typescript-eslint");
const angular = require("@angular-eslint/eslint-plugin");
const angularTemplate = require("@angular-eslint/eslint-plugin-template");
const templateParser = require("@angular-eslint/template-parser");
const prettierConfig = require("eslint-config-prettier");

module.exports = tsEslint.config(
  {
    ignores: ["dist/**", "out-tsc/**", ".angular/**", "*.js"],
  },
  {
    files: ["**/*.ts"],
    extends: [
      eslint.configs.recommended,
      ...tsEslint.configs.strictTypeChecked,
      prettierConfig,
    ],
    plugins: { "@angular-eslint": angular },
    languageOptions: {
      parserOptions: {
        project: ["./tsconfig.app.json", "./tsconfig.spec.json"],
        tsconfigRootDir: __dirname,
      },
    },
    rules: {
      "@angular-eslint/directive-selector": [
        "error",
        { type: "attribute", prefix: "app", style: "camelCase" },
      ],
      "@angular-eslint/component-selector": [
        "error",
        { type: "element", prefix: "app", style: "kebab-case" },
      ],
      "@typescript-eslint/no-explicit-any": "error",
      "@typescript-eslint/no-unused-vars": ["error", { argsIgnorePattern: "^_" }],
    },
  },
  {
    files: ["**/*.html"],
    plugins: { "@angular-eslint/template": angularTemplate },
    languageOptions: { parser: templateParser },
    rules: {
      ...angularTemplate.configs.recommended.rules,
    },
  },
);
