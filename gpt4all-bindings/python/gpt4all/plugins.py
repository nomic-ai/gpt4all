"""
Functions for ChatGPT plugin functionality
"""

import re
from urllib.parse import urljoin, urlparse
import requests
import yaml
import json
import logging


logger: logging.Logger = logging.getLogger(__name__)

# global plugin operation information
plugin_operations = {}


def url_to_openapi_spec(url: str) -> dict:
    """
    Returns the openapi spec from the provided url as a json object.
    """
    raw_text = requests.get(url).text

    try:
        # try loading spec in yaml format
        openapi_spec = yaml.safe_load(raw_text)
    except:
        try:
            # try loading spec in json format
            openapi_spec = json.loads(raw_text)
        except:
            raise ValueError("Invalid OpenAPI spec file")

    return openapi_spec


def get_schema_ref(ref: str, openapi_spec: dict) -> dict:
    """
    Gets the schema reference object from the openapi spec.
    """
    return eval("openapi_spec['" + "']['".join(ref.strip("'").split("/")[1:]) + "']")


def generate_plugin_instructions(plugin_url: str) -> str:
    """
    Create instructions for the specified plugin url.
    Also registers the plugin details to plugin_operations.
    """
    global plugin_operations

    plugin_info = requests.get(plugin_url).json()

    # get the api url containing the openapi spec from plugin_info
    api_url = plugin_info.get("api", {}).get("url")

    if not api_url:
        raise ValueError("Invalid api url")

    if not api_url.startswith(('http://', 'https://')):
        # if relative url, build the api url using the plugin url
        api_url = urljoin(plugin_url, api_url)

    parsed_url = urlparse(api_url)
    server_url = f"{parsed_url.scheme}://{parsed_url.netloc}"

    plugin_name = plugin_info.get("name_for_model")

    openapi_spec = url_to_openapi_spec(api_url)

    instructions = ""
    example_input = "input"

    for path, operations in openapi_spec["paths"].items():
        for method, operation in operations.items():
            operation_id = operation["operationId"]
            operation_description = operation["summary"]

            example_function = operation_id

            operation_parameters = {}

            if "parameters" in operation:
                for parameter in operation["parameters"]:
                    parameter_name = parameter["name"]
                    parameter_description = parameter["description"]
                    parameter_type = parameter["schema"]["type"]

                    example_parameter = parameter["name"]
                    example_input = parameter["description"]

                    operation_parameters[parameter_name] = {
                        "description":parameter_description,
                        "type":parameter_type
                    }

            elif "requestBody" in operation:
                if "$ref" in operation["requestBody"]["content"]["application/json"]["schema"].keys():
                    parameters = get_schema_ref(
                        operation["requestBody"]["content"]["application/json"]["schema"]["$ref"], openapi_spec
                    )["properties"]
                else:
                    parameters = operation["requestBody"]["content"]["application/json"]["schema"]["properties"]

                for parameter_name, properties in parameters.items():
                    example_parameter = parameter_name

                    if "description" in properties:
                        example_input = properties["description"]

                    operation_parameters[parameter_name] = {
                        "description":properties["description"] if "description" in properties else None,
                        "type":properties["type"]
                    }

            else:
                raise ValueError(f"Unable to find required parameters for {operation_id}")

            # convert the parameter details into a json string
            parameters_str = ", ".join(
                [f'"{name}": {info["type"]}' for name, info in operation_parameters.items()]
            )

            # add the operation details to the instructions
            instructions += "\n`" + operation_id + "({" + parameters_str + "})` - " + operation_description

            operation_info = {
                "plugin":{"name":plugin_name, "url":server_url},
                "path":path,
                "method":method,
                "parameters":list(operation_parameters.keys())
            }

            logger.info(f"Registered {plugin_name} operation {operation_id} with the following parameters: {', '.join(operation_parameters.keys())}")

            # add the operation to the list of valid plugin operations
            plugin_operations[operation_id] = operation_info

    instructions = instructions.strip("\n")

    instruction = f"""Below are the instructions for the {openapi_spec['info']['title']} plugin api:
{instructions}
To use the plugin, write a function call to the api, e.g. `{plugin_name}.{example_function}({{"{example_parameter}":"{example_input}"}})`"""

    return instruction


def extract_commands(output: str, functions: list[str]) -> tuple[str, dict]:
    """
    Extracts plugin operations from an output string.
    """
    operations = []

    for function_name in functions:
        function_calls = re.findall(function_name + "\(.*?\)", output)

        for function_call in function_calls:
            try:
                parameters = json.loads(function_call.split("(")[1].split(")")[0])

                logger.info(f"Found {function_name} operation with parameters {parameters}")

                operations.append((function_name, parameters))
            except json.JSONDecodeError:
                continue

    if len(operations) != 0:
        return operations
    else:
        raise ValueError("No plugin operations found")


def get_plugin_response(output: str) -> str:
    """
    Extract plugin operation from the output, then perform operation and return the result.
    """
    # find plugin operations
    all_plugin_operations = extract_commands(output, plugin_operations)

    for operation_id, parameters in all_plugin_operations:
        # get operation details
        operation = plugin_operations.get(operation_id)

        if not operation:
            raise ValueError(f"Operation {operation_id} not found")

        # Construct the URL for the API request
        plugin_name = operation["plugin"]["name"]
        path = operation["path"]
        method = operation["method"]
        api_url = operation["plugin"]["url"]
        url = api_url.rstrip("/") + path.format(**parameters)

        if all(parameter in path for parameter in operation["parameters"]):
            response = requests.request(method, url)
        else:
            response = requests.request(method, url, json=parameters)

        # Check if the response is successful
        if response.ok:
            if response.text.strip():
                logger.info(f"Successfully ran operation {operation_id} with parameters {parameters}.\nThe {plugin_name} plugin returned the following data:\n{response.text}")

                return f"The {plugin_name} plugin api has returned the following output:\n{response.text}"
            else:
                logger.error(f"Ran operation {operation_id} with parameters {parameters} but received an empty response.")
        else:
            logger.error(f"Ran operation {operation_id} with parameters {parameters}, but received the following error:\n{response.content}")

        continue

    raise ValueError("No valid operations were found")
