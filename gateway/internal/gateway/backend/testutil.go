package backend

import (
	"fmt"
	"io/ioutil"
	"os"
	"strings"
	"time"

	gw "github.com/cvmfs/gateway/internal/gateway"
	"github.com/cvmfs/gateway/internal/gateway/receiver"
)

// accessConfigV1 is an access configuration using the legacy syntax
const accessConfigV1 = `
{
	"repos": [
		{
			"domain": "test.repo.org",
			"keys": ["keyid123"]
		}
	],
	"keys": [
		{
			"type": "file",
			"file_name": "/etc/cvmfs/keys/test.repo.org.gw",
			"repo_subpath": "/"
		},
		{
			"type": "plain_text",
			"id": "keyid2",
			"secret": "secret2",
			"repo_subpath": "/"
		}
	]
}
`

// accessConfigV2 is an access configuration using the new syntax
const accessConfigV2 = `
{
	"version": 2,
	"repos" : [
		"test1.repo.org",
		{
			"domain": "test2.repo.org",
			"keys": [
				{
					"id": "keyid1",
					"path": "/"
				},
				{
					"id": "keyid2",
					"path": "/restricted/to/subdir"
				}
			]
		}
	],
	"keys": [
		{
			"type": "file",
			"file_name": "/etc/cvmfs/keys/test2.repo.org.gw"
		},
		{
			"type": "plain_text",
			"id": "keyid2",
			"secret": "secret2"
		}
	]
}
`

// accessConfigV2NoKeys is a minimal access configuration using the new syntax
const accessConfigV2NoKeys = `
{
	"version": 2,
	"repos" : [
		"test1.repo.org"
	]
}
`

const (
	maxLeaseTime time.Duration = 100 * time.Second
)

// mockKeyImporter is used by tests, returns a predefined (id, secret) pair
// instead of reading from file
func mockKeyImporter(ks KeySpec) (string, string, string, error) {
	switch ks.KeyType {
	case "plain_text":
		return ks.ID, ks.Secret, ks.Path, nil
	case "file":
		return "keyid123", "secret123", "/", nil
	default:
		return "", "", "", fmt.Errorf("unknown key type")
	}
}

// testConfig is a set of backend configuration values for use in tests
func testConfig(workDir string) *gw.Config {
	return &gw.Config{
		Port:          4929,
		MaxLeaseTime:  50 * time.Millisecond, // use 50ms leases by default in testing mode
		UseEtcd:       false,
		LogLevel:      "info",
		LogTimestamps: false,
		NumReceivers:  1,
		ReceiverPath:  "/usr/bin/cvmfs_receiver",
		WorkDir:       workDir,
		MockReceiver:  true,
	}
}

// StartTestBackend for testing
func StartTestBackend(name string, maxLeaseTime time.Duration) (*Services, string) {
	tmp, err := ioutil.TempDir("", name)
	if err != nil {
		os.Exit(1)
	}
	cfg := testConfig(tmp)
	cfg.MaxLeaseTime = maxLeaseTime

	ac := emptyAccessConfig()

	rd := strings.NewReader(accessConfigV2)
	if err := ac.load(rd, mockKeyImporter); err != nil {
		os.Exit(2)
	}

	ldb, err := OpenLeaseDB("sqlite", cfg)
	if err != nil {
		os.Exit(3)
	}

	pool, err := receiver.StartPool(cfg.ReceiverPath, cfg.NumReceivers, cfg.MockReceiver)
	if err != nil {
		os.Exit(4)
	}

	return &Services{Access: ac, Leases: ldb, Pool: pool, Config: *cfg}, tmp
}
