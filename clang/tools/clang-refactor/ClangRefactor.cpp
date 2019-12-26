
#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/DiagnosticOptions.h"
#include "clang/Basic/FileManager.h"
#include "clang/Basic/IdentifierTable.h"
#include "clang/Basic/LangOptions.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Basic/TokenKinds.h"
#include "clang/Frontend/TextDiagnosticPrinter.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Refactoring.h"
#include "clang/Tooling/Refactoring/Rename/RenamingAction.h"
#include "clang/Tooling/Refactoring/Rename/USRFindingAction.h"
#include "clang/Tooling/ReplacementsYaml.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/ADT/IntrusiveRefCntPtr.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/YAMLTraits.h"
#include "llvm/Support/raw_ostream.h"
#include <string>
#include <system_error>
#include "clang/AST/ExprCXX.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/AST/RecursiveASTVisitor.h"

using namespace std;
using namespace clang;
using namespace clang::driver;
using namespace clang::tooling;
using namespace llvm;

static cl::OptionCategory MyToolCategory("My tool options");

static cl::opt<std::string>
    OutFilePath("out",
                   cl::desc("The yaml output file path."),cl::cat(MyToolCategory)
                   );

std::vector<AtomicChange> Changes;

static bool isNoexcept(const FunctionDecl* FD) {

	const auto* FPT = FD->getType()->castAs<FunctionProtoType>();
	// This is for make sure I can use FPT->isNothrow(), else it will crash the program and for now I don't know why, and the worst case is
// that a I add noexcept twice.
	switch (FPT->getExceptionSpecType()) {
	case EST_Unparsed:
	case EST_Unevaluated:
	case EST_Uninstantiated:
		return false;
	default:
		break;
	}
	if (FD->hasAttr<NoThrowAttr>() || FPT->isNothrow() || FD->isExternC())
		return true;
	return false;
}

class ExampleVisitor : public RecursiveASTVisitor<ExampleVisitor> {
private:
	ASTContext *astContext; // used for getting additional AST info

public:
	explicit ExampleVisitor(CompilerInstance *CI)
		: astContext(&(CI->getASTContext())) // initialize private members
	{
	}

	virtual bool VisitFunctionDecl(FunctionDecl *func) {
		if (isNoexcept(func) || func->getLocation().isMacroID()  || func->isImplicit())
		{
			return true;
		}
		auto& SourceManager = astContext->getSourceManager();
		auto ExpansionLoc = SourceManager.getExpansionLoc(func->getBeginLoc());
		if (ExpansionLoc.isInvalid()) {
			return false;
		}
		if(SourceManager.isInSystemHeader(ExpansionLoc))
		{
			return false;
		}
		bool Error = false;
		std::string text =
			Lexer::getSourceText(CharSourceRange::getTokenRange(
				func->getEndLoc()),
				SourceManager, LangOptions(), &Error);
		int firstWordStartIndex=0;
		for( ;firstWordStartIndex< text.size(); firstWordStartIndex++)
		{
			if(text[firstWordStartIndex] != '\n' && text[firstWordStartIndex] != '\r' && text[firstWordStartIndex] != ' ' && text[firstWordStartIndex
				] !='\t')
			{
				break;
			}
		}
		if(firstWordStartIndex == text.size())
		{
			return true;
		}

		int wordLastIndex = firstWordStartIndex + 1;
		for (; wordLastIndex < text.size(); wordLastIndex++)
		{
			if (text[wordLastIndex] != '\n' && text[wordLastIndex] != '\r' && text[wordLastIndex] != ' ' && text[wordLastIndex
			] != '\t' && (text[wordLastIndex] < 'A' || text[wordLastIndex] > 'Z'))
			{
				break;
			}
		}

		std::string wordAfterFunction = text.substr(firstWordStartIndex, wordLastIndex - firstWordStartIndex);
		int shouldAddToOffset = firstWordStartIndex;
		if(wordAfterFunction == ")")
		{
			shouldAddToOffset += 1;
		}
		else if (wordAfterFunction == "c")
		{
			shouldAddToOffset += std::string("const").size();
		}
		else if (wordAfterFunction == "t")
		{
			shouldAddToOffset -= std::string("= default").size();
		}
		else if (wordAfterFunction == "0")
		{
			shouldAddToOffset -= std::string("= ").size();
		}
		else if(wordAfterFunction == "e")
		{
			shouldAddToOffset -= 1;
		}
		
		auto location = func->getEndLoc().getLocWithOffset(shouldAddToOffset);
		if(func->isThisDeclarationADefinition() && func->getBody())
		{
			location = func->getBody()->getBeginLoc().getLocWithOffset(-1);
		}
		AtomicChange Change(astContext->getSourceManager(), location);
		
			
		Change.insert(astContext->getSourceManager(), location,
			" noexcept ");
		Changes.push_back(Change);
		
		return true;
	}

};



class ExampleASTConsumer : public ASTConsumer {
private:
    ExampleVisitor *visitor; // doesn't have to be private

public:
    // override the constructor in order to pass CI
    explicit ExampleASTConsumer(CompilerInstance *CI)
        : visitor(new ExampleVisitor(CI)) // initialize the visitor
    { }

    // override this to call our ExampleVisitor on the entire source file
    virtual void HandleTranslationUnit(ASTContext &Context) {
        /* we can use ASTContext to get the TranslationUnitDecl, which is
             a single Decl that collectively represents the entire source file */
        visitor->TraverseDecl(Context.getTranslationUnitDecl());
    }

/*
    // override this to call our ExampleVisitor on each top-level Decl
    virtual bool HandleTopLevelDecl(DeclGroupRef DG) {
        // a DeclGroupRef may have multiple Decls, so we iterate through each one
        for (DeclGroupRef::iterator i = DG.begin(), e = DG.end(); i != e; i++) {
            Decl *D = *i;    
            visitor->TraverseDecl(D); // recursively visit each AST node in Decl "D"
        }
        return true;
    }
*/
};



class ExampleFrontendAction : public ASTFrontendAction {
public:
    virtual std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI, StringRef file) {
        return std::make_unique<ExampleASTConsumer>(&CI); // pass CI pointer to ASTConsumer
    }
};


int main(int argc, const char **argv) {
    // parse the command-line args passed to your code
    CommonOptionsParser op(argc, argv, MyToolCategory);        
    // create a new Clang Tool instance (a LibTooling environment)
    ClangTool Tool(op.getCompilations(), op.getSourcePathList());

    // run the Clang Tool, creating a new FrontendAction (explained below)
    int result = Tool.run(newFrontendActionFactory<ExampleFrontendAction>().get());
	if (OutFilePath.size() == 0) {
		llvm::errs() << "Need yaml output file path " << '\n';
		cl::PrintHelpMessage();
		return -1;
	}
	std::string ExportFixes = OutFilePath;
	if (!ExportFixes.empty()) {
		std::error_code EC;
		llvm::raw_fd_ostream OS(ExportFixes, EC, llvm::sys::fs::OF_None);
		if (EC) {
			llvm::errs() << "Error opening output file: " << EC.message() << '\n';
			return 1;
		}
		// Export replacements.
		tooling::TranslationUnitReplacements TUR;
		std::vector<Replacement> replacements;
		for (auto& change: Changes)
		{
			for(auto& replacement : change.getReplacements())
			{
				replacements.push_back(replacement);
			}
		}
		TUR.Replacements.insert(TUR.Replacements.end(), replacements.begin(),
			replacements.end());

	    llvm::yaml::Output YAML(OS);
		YAML << TUR;
		OS.close();
		return result;
	}
}
