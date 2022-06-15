#include "fbrpc/core/ssLogger.h"
#include "fbrpc/generator/ssUtils.h"

#include "ssTSPrinter.h"
#include "ssTSComponentGenerator.h"
#include "ssTSUtils.h"

namespace fbrpc
{

    void sTSComponentGenerator::importTypes(sTSPrinter& printer)
    {
        printer.addImport("{ setComponentProperty, setComponentProperties }", "./ComponentPropertyHelper");
        printer.addImport("{ ComponentProperties }", "./ComponentTypes");
        printer.addContent("type ComponentPropertyPair<T extends ComponentProperties> = [T['type'], T['properties']]");
    }

    bool sTSComponentGenerator::generate(
        sLanguageGenerator::sWriteFileDelegate writter, sTSPrinter& printer, flatbuffers::ServiceDef* service, flatbuffers::RPCCall* call)
    {
        auto componentContext = getComponentContext(call);
        if (componentContext.types.empty())
            return false;

        if (!generateComponentPropertyHelper(writter, service, call, componentContext))
            return false;

        if (!generateComponentTypes(writter, service, call, componentContext))
            return false;

        printer.addContent(R"#(
static setComponentProperty(objectIds: string[], ...args: ComponentPropertyPair<ComponentProperties>): Promise<SetComponentPropertyResponseT> {
	return setComponentProperty(objectIds, ...args);
}
static setComponentProperties(req: {object: string, components: ComponentProperties[]}[]): Promise<SetComponentPropertiesResponseT> {
	return setComponentProperties(req);
}
)#");
        return true;
    }

    sTSComponentGenerator::sComponentContext sTSComponentGenerator::getComponentContext(flatbuffers::RPCCall* call)
    {
        std::map<std::string, std::vector<flatbuffers::FieldDef*>> dependentTypes;
        auto addDependentType = [&dependentTypes](const flatbuffers::EnumVal& enumValue) {
            if (dependentTypes.find(enumValue.name) == dependentTypes.end())
            {
                std::vector<flatbuffers::FieldDef*> propertyFields;
                if (enumValue.union_type.struct_def)
                    for (const auto& propertyValue : enumValue.union_type.struct_def->fields.vec)
                        propertyFields.push_back(propertyValue);

                dependentTypes[enumValue.name] = propertyFields;
            }
        };

        auto& requestFields = call->request->fields.vec;
        auto componentField = std::find_if(
            requestFields.begin(),
            requestFields.end(),
            [](auto&& field) { return field->value.type.base_type == flatbuffers::BASE_TYPE_STRUCT; });

        if (componentField == requestFields.end())
        {
            logger().error("generate component setter failed, no component field");
            return {};
        }

        auto componentStructDef = (*componentField)->value.type.struct_def;
        std::string ns = generatorUtils::toNameSpacePath(componentStructDef->defined_namespace);

        for (const auto& structDefField : componentStructDef->fields.vec)
            if (structDefField->value.type.base_type == flatbuffers::BASE_TYPE_UNION && structDefField->value.type.enum_def)
                for (const auto& enumValue : structDefField->value.type.enum_def->Vals())
                    if (enumValue->union_type.base_type != flatbuffers::BASE_TYPE_NONE)
                        addDependentType(*enumValue);

        return {ns, dependentTypes};
    }

    bool sTSComponentGenerator::generateComponentPropertyHelper(
        sLanguageGenerator::sWriteFileDelegate writter,
        flatbuffers::ServiceDef* service,
        flatbuffers::RPCCall* call,
        const sComponentContext& context)
    {
        auto serviceName = service->name;

        sTSPrinter printer;
        printer.addHeader();

        printer.addImport("* as flatbuffers", "flatbuffers");
        printer.addImport(std::string("{ ") + serviceName + " }", std::string("./") + sTSUtils::BindingTargetName.data());
        printer.addImport(
            std::string("{ ") + call->request->name + " }",
            sTSUtils::toFullPath(call->request));
        printer.addImport(
            std::string("{ ") + call->response->name + ", " + call->response->name + "T }",
            sTSUtils::toFullPath(call->response));

        printer.addContent("import { SetComponentPropertiesResponse, SetComponentPropertiesResponseT } from './editor-service/set-component-properties-response'");
        printer.addContent("import { SetComponentPropertiesRequest } from './editor-service/set-component-properties-request'");
        printer.addContent("import { SingleSetComponentPropertyRequest } from './editor-service/single-set-component-property-request'");

        importComponentTypes(printer, context);

        printer.addImport("{ Plain }", "./FromPlain");
        for (const auto& [type, _] : context.types)
            printer.addImport(std::string("{ fromPlain" + type + "T }"), "./FromPlain");

        printer.addContent("import { IGetValue } from './internal'");

        printer.nextLine();

        std::string createSetterCodeSnippet("switch (type) {");
        for (const auto& [type, _] : context.types)
        {
            createSetterCodeSnippet += printer.format(
                R"#(
        case fbComponent.$type$: {
            input = fromPlain$type$T(args as Plain<$type$T>);
            setter = new $type$Setter(builder) as any; 
            break;
        })#",
                {{"type", type}});
        }
        createSetterCodeSnippet += "    default: break;\n    }\n";

        sPrinter::sVarsMap vars = {
            {"createSetterCodeSnippet", createSetterCodeSnippet}};

        printer.addContent(
            R"#(
export async function setComponentProperty(objectIds: string[], type: any, args: any): Promise<SetComponentPropertyResponseT> {
	let builder = new flatbuffers.Builder();
	let offset = await serializeComponentProperty(builder, type, args);
	return await sendSetComponentPropertyRequest(builder, objectIds, offset);
}
export async function setComponentProperties(requests: any[]): Promise<SetComponentPropertiesResponseT> {
    const builder = new flatbuffers.Builder();
    const requestOffsets = [];
    for (const request of requests) {
        const {object, components} = request;
        const componentOffsets = [];
        for (const component of components) {
            const type = component.type;
            componentOffsets.push(await serializeComponentProperty(builder, type, component.properties))
        }
        const objectOffset = builder.createString(object);
		const componentOffsetVector = SingleSetComponentPropertyRequest.createComponentsVector(builder, componentOffsets);
        SingleSetComponentPropertyRequest.startSingleSetComponentPropertyRequest(builder);
        SingleSetComponentPropertyRequest.addObject(builder, objectOffset);
        SingleSetComponentPropertyRequest.addComponents(builder, componentOffsetVector);
        const offset = SingleSetComponentPropertyRequest.endSingleSetComponentPropertyRequest(builder);
        requestOffsets.push(offset);
    }
	const requestOffsetVector = SetComponentPropertiesRequest.createRequestsVector(builder, requestOffsets);
    SetComponentPropertiesRequest.startSetComponentPropertiesRequest(builder);
    SetComponentPropertiesRequest.addRequests(builder, requestOffsetVector);
    const offset = SetComponentPropertiesRequest.endSetComponentPropertiesRequest(builder);
    builder.finish(offset);

    return new Promise(resolve => {
        Sandbox.setComponentProperties(builder.asUint8Array(), res => {
            let buffer = new flatbuffers.ByteBuffer(res);
            let response = SetComponentPropertiesResponse.getRootAsSetComponentPropertiesResponse(buffer);
            resolve(response.unpack());
        });
    })
}
async function serializeComponentProperty(builder: flatbuffers.Builder, type: any, args: any): Promise<flatbuffers.Offset> {
	let setter: any;
	let input: any;
    $createSetterCodeSnippet$
    for(let key of Object.keys(args))
        setter[key] = input[key];
    let offset = await setter.finish();
	return offset;
}
)#",
            vars);
        printer.addContent(
            R"#(async function sendSetComponentPropertyRequest(builder: flatbuffers.Builder, ids: string[], componentOffset: flatbuffers.Offset): Promise<SetComponentPropertyResponseT>{
	return new Promise(resolve => {
		const idsOffset = SetComponentPropertyRequest.createIdsVector(builder, builder.createObjectOffsetList(ids));
		SetComponentPropertyRequest.startSetComponentPropertyRequest(builder);
		SetComponentPropertyRequest.addIds(builder, idsOffset);
		SetComponentPropertyRequest.addComponent(builder, componentOffset);
		builder.finish(SetComponentPropertyRequest.endSetComponentPropertyRequest(builder));
		Sandbox.setComponentProperty(builder.asUint8Array(), res => {
			let buffer = new flatbuffers.ByteBuffer(res);
			let response = SetComponentPropertyResponse.getRootAsSetComponentPropertyResponse(buffer);
			resolve(response.unpack());
		});
	});
})#");
        printer.addContent(
            R"#(class BaseSetter {
	constructor(public builder: flatbuffers.Builder) {}
	valueOrOffsetMap = new Map();
	addSettedProperties() {
		this.builder.forceDefaults(true);
		for (let key of this.valueOrOffsetMap.keys())
			(this as any)["add_" + key](this.valueOrOffsetMap.get(key));
		this.builder.forceDefaults(false);
	}
})#");
        for (const auto& depComp : context.types)
        {
            std::string propertyCodeSnippet;
            std::string propertySetterCodeSnippet;
            for (const flatbuffers::FieldDef* propertyDef : depComp.second)
            {
                if (propertyDef->deprecated)
                    continue;

                auto baseType = propertyDef->value.type;
                std::string propertyName = propertyDef->name;
                auto typeName = sTSUtils::genTypeName(propertyDef->value.type);

                std::string compName = depComp.first;
                std::string propertyNameCamel = generatorUtils::toCamel(propertyDef->name, true);
                std::string genOffsetOrValue = sTSUtils::genOffsetOrValue(propertyDef->value.type, compName, propertyNameCamel);

                auto propertyCodeSnippetTemplate = R"#(
    set $propertyName$(value: $typeName$) {
        if (value !== null)
            this.valueOrOffsetMap.set("$propertyName$", $genOffsetOrValue$);
	}
)#";
                propertyCodeSnippet += printer.format(
                    propertyCodeSnippetTemplate,
                    {{"propertyName", propertyName}, {"typeName", typeName}, {"genOffsetOrValue", genOffsetOrValue}});

                bool needOffset = !flatbuffers::IsScalar(baseType.base_type) && !flatbuffers::IsStruct(baseType);

                std::string offsetOrValueParam = needOffset ? "offset: number" : "value: " + sTSUtils::genTypeName(propertyDef->value.type);
                std::string offsetOrValue = needOffset ? "offset" : (flatbuffers::IsStruct(propertyDef->value.type) ? "value.pack(this.builder)" : "value");

                auto propertySetterCodeSnippetTemplate = R"#(
    add_$propertyName$($offsetOrValueParam$) {
        $compName$.add$propertyNameCamel$(this.builder, $offsetOrValue$);
    }
)#";
                propertySetterCodeSnippet += printer.format(
                    propertySetterCodeSnippetTemplate,
                    {{"propertyName", propertyName},
                     {"offsetOrValueParam", offsetOrValueParam},
                     {"compName", compName},
                     {"propertyNameCamel", propertyNameCamel},
                     {"offsetOrValue", offsetOrValue}});
            }

            sPrinter::sVarsMap vars = {
                {"componentType", depComp.first},
                {"propertyCodeSnippet", propertyCodeSnippet},
                {"propertySetterCodeSnippet", propertySetterCodeSnippet}};

            printer.addContent(
                R"#(class $componentType$Setter extends BaseSetter {
	constructor(public builder: flatbuffers.Builder) { super(builder); }
    async finish(): Promise<flatbuffers.Offset> {
        $componentType$.start$componentType$(this.builder);
		super.addSettedProperties();
        const componentUnionOffset = $componentType$.end$componentType$(this.builder);
        const componentOffset = Component.createComponent(
            this.builder,
            fbComponent.$componentType$,
            componentUnionOffset
        );
        return componentOffset;
    }
$propertyCodeSnippet$
$propertySetterCodeSnippet$
})#",
                vars);
        }

        printer.nextLine();

        return writter(printer.getOutput(), "ComponentPropertyHelper.ts");
    }

    bool sTSComponentGenerator::generateComponentTypes(
        sLanguageGenerator::sWriteFileDelegate writter,
        flatbuffers::ServiceDef* service,
        flatbuffers::RPCCall* call,
        const sComponentContext& context)
    {
        sTSPrinter componentTypes;
        componentTypes.addHeader();
        importComponentTypes(componentTypes, context);
        componentTypes.addContent("import { IGetValue } from './internal'");
        componentTypes.addContent("import { Plain } from './FromPlain'");

        componentTypes.addContent("export type ComponentTypeMap = {");
        for (const auto& [type, _] : context.types)
            componentTypes.addContent("    [fbComponent.$type$]: Plain<$type$T>,", {{"type", type}});

        componentTypes.addContent("}");

        componentTypes.addContent("export type ComponentProperties = { [K in keyof ComponentTypeMap]: { type: K, properties: Partial<IGetValue<ComponentTypeMap, K>>} }[keyof ComponentTypeMap]");
        return writter(componentTypes.getOutput(), "ComponentTypes.ts");
    }

    void sTSComponentGenerator::importComponentTypes(sTSPrinter& printer, const sComponentContext& context)
    {
        auto nameSpaceDir = generatorUtils::toDasherizedCase(context.ns);
        printer.addContent("import { Component } from './" + nameSpaceDir + "/component';");
        printer.addContent("import { fbComponent } from './" + nameSpaceDir + "/fb-component';");

        std::set<std::string> importedClass;
        for (const auto& type : context.types)
        {
            auto target = std::string("{ ") + type.first + ", " + type.first + "T" + " }";
            auto from = std::string("./") + nameSpaceDir + "/" + generatorUtils::toDasherizedCase(type.first);
            printer.addImport(target, from);

            for (const flatbuffers::FieldDef* propertyDef : type.second)
            {
                auto& type = propertyDef->value.type;
                if (type.enum_def && importedClass.find(type.enum_def->name) == importedClass.end())
                {
                    importedClass.insert(type.enum_def->name);
                    std::string enumTarget = std::string("{ ") + type.enum_def->name + " }";
                    std::string enumFrom = sTSUtils::toFullPath(type.enum_def);
                    printer.addImport(enumTarget, enumFrom);
                }
                if (type.struct_def && importedClass.find(type.struct_def->name) == importedClass.end())
                {
                    importedClass.insert(type.struct_def->name);
                    std::string structTarget = std::string("{ ") + type.struct_def->name + 'T' + " }";
                    std::string structFrom = sTSUtils::toFullPath(type.struct_def);
                    printer.addImport(structTarget, structFrom);
                }
            }
        }
    }
}