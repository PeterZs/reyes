
ParserPrototype = TargetPrototype {
    "Parser", BIND_PHONY,

    static_depend = function( self )
        if built_for_platform_and_variant(self) then        
            for _, value in ipairs(self) do
                local grammar = SourceFile( value );    
                grammar:set_required_to_exist( true );
                grammar.unit = self;
                local source = HeaderFile( basename(value)..".hpp" );
                source:set_bind_type( BIND_INTERMEDIATE_FILE );
                source.grammar = value;
                grammar.object = source;
                source:add_dependency( grammar );
                self:add_dependency( source );
            end
        end    
    end;

    build = function( self )
        if self:is_outdated() and built_for_platform_and_variant(self) then
            local parser = self.settings.parser.executable;
            for dependency in self:get_dependencies() do
                if dependency:is_outdated() then
                    print( dependency.grammar );
                    local arguments = "parser -r parser/cxx "..dependency.grammar;
                    if exec(parser, arguments) ~= 0 then
                        error( arguments.." failed" );
                    end
                end
            end
        end
    end;

    clean = function( self )
        if built_for_platform_and_variant(self) then    
            for dependency in self:get_dependencies() do
                if dependency:prototype() == HeaderFilePrototype then
                    rm( dependency:path() );
                end
            end
        end
    end;
}

function parser( settings )
    putenv( "LUA_PATH", settings.parser.lua_path );
end

function Parser( parser )
    assert( type(parser) == "table" );
    return target( "", ParserPrototype, parser );
end
